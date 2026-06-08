#include "app_update.h"
#include "boot_crc32.h"
#include "boot_image.h"
#include "boot_param.h"
#include "bsp_spi_flash.h"
#include "bsp_tf.h"
#include "fatfs/ff.h"

#define APP_UPDATE_BUFFER_SIZE 512U

static uint8_t s_update_buffer[APP_UPDATE_BUFFER_SIZE];
static app_update_progress_callback_t s_progress_callback;

/* 注册 App 侧升级流程进度回调。
 * 参数：callback 为进度回调函数，传入 0 表示取消回调。
 * 返回：无。
 */
void app_update_set_progress_callback(app_update_progress_callback_t callback)
{
    s_progress_callback = callback;
}

/* 上报当前固件暂存流程进度。
 * 参数：progress 为阶段；done 为已完成数量；total 为总数量。
 * 返回：无。
 */
static void app_update_emit_progress(app_update_progress_t progress, uint32_t done, uint32_t total)
{
    if (s_progress_callback != 0) {
        s_progress_callback(progress, done, total);
    }
}

/* 按扇区擦除外部 SPI FLASH 的固件暂存区域。
 * 参数：address 为起始地址；length 为需要覆盖的字节数。
 * 返回：1 表示擦除完成，0 表示任一扇区擦除失败。
 */
static uint8_t app_update_ext_flash_erase(uint32_t address, uint32_t length)
{
    uint32_t erase_addr;
    uint32_t end;
    uint32_t done;
    uint32_t total;

    if (length == 0U) {
        return 1U;
    }

    erase_addr = address & ~(BSP_SPI_FLASH_SECTOR_SIZE - 1U);
    end = address + length;
    done = 0U;
    total = (end - erase_addr + BSP_SPI_FLASH_SECTOR_SIZE - 1U) / BSP_SPI_FLASH_SECTOR_SIZE;
    app_update_emit_progress(APP_UPDATE_PROGRESS_ERASE, done, total);

    while (erase_addr < end) {
        /* 外部 FLASH 只能按扇区擦除，所以先向下对齐到扇区边界。 */
        if (bsp_spi_flash_sector_erase(erase_addr) == 0U) {
            return 0U;
        }
        erase_addr += BSP_SPI_FLASH_SECTOR_SIZE;
        done++;
        app_update_emit_progress(APP_UPDATE_PROGRESS_ERASE, done, total);
    }

    return 1U;
}

/* 生成外部 FLASH 镜像头。
 * 参数：header 为输出头；image_size/image_crc32 为 App 镜像信息；version 为版本号；stack_addr/entry_addr 为向量表前两项。
 * 返回：无。
 */
static void app_update_build_header(boot_image_header_t *header, uint32_t image_size, uint32_t image_crc32, uint32_t version, uint32_t stack_addr, uint32_t entry_addr)
{
    uint32_t index;

    header->magic         = BOOT_IMAGE_MAGIC;
    header->header_size   = sizeof(boot_image_header_t);
    header->header_crc32  = 0U;
    header->image_size    = image_size;
    header->image_crc32   = image_crc32;
    header->image_version = version;
    header->target_addr   = BOOT_APP_START_ADDR;
    header->stack_addr    = stack_addr;
    header->entry_addr    = entry_addr;
    header->state         = BOOT_IMAGE_STATE_READY;
    header->sequence      = 0U;
    header->tail_magic    = BOOT_IMAGE_TAIL_MAGIC;

    for (index = 0U; index < (sizeof(header->reserved) / sizeof(header->reserved[0])); index++) {
        header->reserved[index] = 0xFFFFFFFFU;
    }

    header->header_crc32 = boot_image_header_crc32(header);
}

/* 从 TF 卡读取固件文件并暂存到外部 SPI FLASH。
 * 参数：path 为固件路径，传入 0 使用默认 firmware.bin；version 为待写入镜像版本；out_header 可选输出镜像头。
 * 返回：APP_UPDATE_OK 表示暂存成功，其余状态指示挂载、读取、校验或参数写入失败。
 */
app_update_status_t app_update_stage_tf_firmware(const char *path, uint32_t version, boot_image_header_t *out_header)
{
    FIL file;
    FRESULT result;
    UINT bytes_read;
    uint32_t file_size;
    uint32_t offset;
    uint32_t crc;
    uint32_t stack_addr;
    uint32_t entry_addr;
    boot_image_header_t header;

    if (path == 0) {
        path = BOOT_TF_FIRMWARE_PATH;
    }

    /* 先完成 TF 挂载和文件打开，后续才擦除外部 FLASH，避免文件不存在时破坏暂存区。 */
    app_update_emit_progress(APP_UPDATE_PROGRESS_MOUNT, 0U, 0U);
    if (bsp_tf_mount() != BSP_TF_OK) {
        return APP_UPDATE_TF_MOUNT_ERR;
    }

    app_update_emit_progress(APP_UPDATE_PROGRESS_OPEN, 0U, 0U);
    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        return APP_UPDATE_TF_OPEN_ERR;
    }

    file_size = (uint32_t)f_size(&file);
    if (file_size == 0U) {
        (void)f_close(&file);
        return APP_UPDATE_FILE_EMPTY;
    }

    if (file_size < 8U) {
        (void)f_close(&file);
        return APP_UPDATE_VECTOR_ERR;
    }

    if ((file_size > BOOT_APP_MAX_SIZE) || ((file_size + BOOT_IMAGE_HEADER_BYTES) > BOOT_EXT_SLOT0_SIZE)) {
        (void)f_close(&file);
        return APP_UPDATE_FILE_TOO_LARGE;
    }

    if (app_update_ext_flash_erase(BOOT_EXT_SLOT0_ADDR, file_size + BOOT_IMAGE_HEADER_BYTES) == 0U) {
        (void)f_close(&file);
        return APP_UPDATE_FLASH_ERASE_ERR;
    }

    offset = 0U;
    crc = boot_crc32_start();
    stack_addr = 0U;
    entry_addr = 0U;
    app_update_emit_progress(APP_UPDATE_PROGRESS_COPY, offset, file_size);

    while (offset < file_size) {
        result = f_read(&file, s_update_buffer, sizeof(s_update_buffer), &bytes_read);
        if ((result != FR_OK) || (bytes_read == 0U)) {
            (void)f_close(&file);
            return APP_UPDATE_TF_READ_ERR;
        }

        if (offset == 0U) {
            /* .bin 文件起始 8 字节是 Cortex-M 向量表，用来判断是否是可跳转 App。 */
            stack_addr = ((uint32_t)s_update_buffer[0]) |
                         ((uint32_t)s_update_buffer[1] << 8U) |
                         ((uint32_t)s_update_buffer[2] << 16U) |
                         ((uint32_t)s_update_buffer[3] << 24U);
            entry_addr = ((uint32_t)s_update_buffer[4]) |
                         ((uint32_t)s_update_buffer[5] << 8U) |
                         ((uint32_t)s_update_buffer[6] << 16U) |
                         ((uint32_t)s_update_buffer[7] << 24U);
        }

        if (bsp_spi_flash_write(BOOT_EXT_SLOT0_DATA_ADDR + offset, s_update_buffer, bytes_read) == 0U) {
            (void)f_close(&file);
            return APP_UPDATE_FLASH_WRITE_ERR;
        }

        crc = boot_crc32_update(crc, s_update_buffer, bytes_read);
        offset += bytes_read;
        app_update_emit_progress(APP_UPDATE_PROGRESS_COPY, offset, file_size);
    }

    (void)f_close(&file);
    crc = boot_crc32_finish(crc);

    if (boot_is_valid_app_vector(stack_addr, entry_addr) == 0U) {
        return APP_UPDATE_VECTOR_ERR;
    }

    app_update_build_header(&header, file_size, crc, version, stack_addr, entry_addr);
    if (bsp_spi_flash_write(BOOT_EXT_SLOT0_ADDR, (const uint8_t *)&header, sizeof(header)) == 0U) {
        return APP_UPDATE_FLASH_WRITE_ERR;
    }

    app_update_emit_progress(APP_UPDATE_PROGRESS_VERIFY, 0U, 0U);
    if (boot_external_image_validate(BOOT_EXT_SLOT0_ADDR, &header) != BOOT_OK) {
        return APP_UPDATE_VERIFY_ERR;
    }

    app_update_emit_progress(APP_UPDATE_PROGRESS_PARAM, 0U, 0U);
    /* 标记 pending 后，下一次进入 Bootloader 时会把外部 FLASH 镜像复制进内部 App 区。 */
    if (boot_param_mark_pending(BOOT_EXT_SLOT0_ADDR, file_size, crc, version) == 0U) {
        return APP_UPDATE_PARAM_ERR;
    }

    if (out_header != 0) {
        *out_header = header;
    }

    return APP_UPDATE_OK;
}
