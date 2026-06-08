#include "boot_image.h"
#include "boot_crc32.h"
#include "boot_flash.h"
#include "boot_param.h"
#include "bsp_spi_flash.h"
#include "systick.h"

#define BOOT_COPY_BUFFER_SIZE 512U

static uint8_t s_boot_buffer[BOOT_COPY_BUFFER_SIZE];

/* 检查 App 向量表前两项是否可信。
 * 参数：stack_addr 为初始 MSP；entry_addr 为复位入口地址。
 * 返回：1 表示栈地址和入口地址都位于允许范围，0 表示非法。
 */
uint8_t boot_is_valid_app_vector(uint32_t stack_addr, uint32_t entry_addr)
{
    uint32_t entry_aligned;

    entry_aligned = entry_addr & ~1U;

    if ((stack_addr < BOOT_SRAM_START_ADDR) || (stack_addr > BOOT_SRAM_END_ADDR)) {
        return 0U;
    }

    if ((entry_addr & 1U) == 0U) {
        return 0U;
    }

    if ((entry_aligned < BOOT_APP_START_ADDR) || (entry_aligned >= BOOT_APP_END_ADDR)) {
        return 0U;
    }

    return 1U;
}

/* 计算镜像头自身的 CRC32。
 * 参数：header 为待计算的镜像头。
 * 返回：header_crc32 字段清零后的 CRC32。
 */
uint32_t boot_image_header_crc32(const boot_image_header_t *header)
{
    boot_image_header_t temp;

    if (header == 0) {
        return 0U;
    }

    temp = *header;
    temp.header_crc32 = 0U;
    return boot_crc32_calc((const uint8_t *)&temp, sizeof(temp));
}

/* 校验镜像头字段是否合法。
 * 参数：header 为待检查的镜像头。
 * 返回：1 表示魔术字、大小、状态、向量表和头 CRC 均正确，0 表示非法。
 */
uint8_t boot_image_header_is_valid(const boot_image_header_t *header)
{
    if (header == 0) {
        return 0U;
    }

    if ((header->magic != BOOT_IMAGE_MAGIC) ||
        (header->tail_magic != BOOT_IMAGE_TAIL_MAGIC) ||
        (header->header_size != sizeof(boot_image_header_t)) ||
        (header->state != BOOT_IMAGE_STATE_READY)) {
        return 0U;
    }

    if ((header->image_size == 0U) || (header->image_size > BOOT_APP_MAX_SIZE)) {
        return 0U;
    }

    if (header->target_addr != BOOT_APP_START_ADDR) {
        return 0U;
    }

    if (boot_is_valid_app_vector(header->stack_addr, header->entry_addr) == 0U) {
        return 0U;
    }

    return header->header_crc32 == boot_image_header_crc32(header);
}

/* 从外部 SPI FLASH 指定槽位读取并校验镜像头。
 * 参数：slot_addr 为槽位起始地址；header 为输出镜像头。
 * 返回：BOOT_OK 表示读取成功，其余值表示读失败或镜像头非法。
 */
boot_status_t boot_external_image_read_header(uint32_t slot_addr, boot_image_header_t *header)
{
    if (header == 0) {
        return BOOT_ERR_IMAGE_HEADER;
    }

    if (bsp_spi_flash_read(slot_addr, (uint8_t *)header, sizeof(boot_image_header_t)) == 0U) {
        return BOOT_ERR_FLASH_READ;
    }

    if (boot_image_header_is_valid(header) == 0U) {
        return BOOT_ERR_IMAGE_HEADER;
    }

    return BOOT_OK;
}

/* 校验外部 SPI FLASH 中完整 App 镜像。
 * 参数：slot_addr 为槽位起始地址；header 可选输出镜像头，传入 0 时仅校验。
 * 返回：BOOT_OK 表示镜像头和镜像 CRC 均正确。
 */
boot_status_t boot_external_image_validate(uint32_t slot_addr, boot_image_header_t *header)
{
    boot_status_t status;
    boot_image_header_t local_header;
    uint32_t crc;
    uint32_t remaining;
    uint32_t source_addr;
    uint32_t chunk;

    if (header == 0) {
        header = &local_header;
    }

    status = boot_external_image_read_header(slot_addr, header);
    if (status != BOOT_OK) {
        return status;
    }

    crc = boot_crc32_start();
    remaining = header->image_size;
    source_addr = slot_addr + BOOT_IMAGE_HEADER_BYTES;

    while (remaining != 0U) {
        /* 分块读取外部 Flash，避免一次性占用过多 RAM。 */
        chunk = remaining;
        if (chunk > sizeof(s_boot_buffer)) {
            chunk = sizeof(s_boot_buffer);
        }

        if (bsp_spi_flash_read(source_addr, s_boot_buffer, chunk) == 0U) {
            return BOOT_ERR_FLASH_READ;
        }

        crc = boot_crc32_update(crc, s_boot_buffer, chunk);
        source_addr += chunk;
        remaining -= chunk;
    }

    if (boot_crc32_finish(crc) != header->image_crc32) {
        return BOOT_ERR_IMAGE_CRC;
    }

    return BOOT_OK;
}

/* 将外部 SPI FLASH 中的已校验镜像复制到内部 App 区。
 * 参数：slot_addr 为外部槽位起始地址；applied_header 可选输出已安装镜像头。
 * 返回：BOOT_OK 表示升级成功，其余值表示校验、擦写或参数保存失败。
 */
boot_status_t boot_apply_external_image(uint32_t slot_addr, boot_image_header_t *applied_header)
{
    boot_image_header_t header;
    boot_status_t status;
    uint32_t remaining;
    uint32_t source_addr;
    uint32_t target_addr;
    uint32_t chunk;
    uint32_t app_crc;

    status = boot_external_image_validate(slot_addr, &header);
    if (status != BOOT_OK) {
        return status;
    }

    /* 擦写内部 App 区期间关闭中断，避免跳转表或 Flash 操作被中断打断。 */
    __disable_irq();

    if (boot_flash_erase(BOOT_APP_START_ADDR, header.image_size) == 0U) {
        __enable_irq();
        return BOOT_ERR_FLASH_ERASE;
    }

    remaining = header.image_size;
    source_addr = slot_addr + BOOT_IMAGE_HEADER_BYTES;
    target_addr = BOOT_APP_START_ADDR;

    while (remaining != 0U) {
        /* 外部 Flash -> RAM 缓冲 -> 内部 Flash，保证升级过程只依赖小块 RAM。 */
        chunk = remaining;
        if (chunk > sizeof(s_boot_buffer)) {
            chunk = sizeof(s_boot_buffer);
        }

        if (bsp_spi_flash_read(source_addr, s_boot_buffer, chunk) == 0U) {
            __enable_irq();
            return BOOT_ERR_FLASH_READ;
        }

        if (boot_flash_write(target_addr, s_boot_buffer, chunk) == 0U) {
            __enable_irq();
            return BOOT_ERR_FLASH_WRITE;
        }

        source_addr += chunk;
        target_addr += chunk;
        remaining -= chunk;
    }

    __enable_irq();

    app_crc = boot_flash_crc32(BOOT_APP_START_ADDR, header.image_size);
    if (app_crc != header.image_crc32) {
        return BOOT_ERR_IMAGE_CRC;
    }

    if (boot_param_mark_app_installed(&header) == 0U) {
        return BOOT_ERR_PARAM;
    }

    if (applied_header != 0) {
        *applied_header = header;
    }

    return BOOT_OK;
}

/* 判断内部 App 区是否存在可跳转程序。
 * 参数：无。
 * 返回：1 表示向量表有效，0 表示无 App 或 App 已损坏。
 */
uint8_t boot_app_is_present(void)
{
    uint32_t stack_addr;
    uint32_t entry_addr;

    stack_addr = *((const uint32_t *)BOOT_APP_START_ADDR);
    entry_addr = *((const uint32_t *)(BOOT_APP_START_ADDR + 4U));

    return boot_is_valid_app_vector(stack_addr, entry_addr);
}

/* 关闭 Bootloader 运行环境并跳转到 App。
 * 参数：app_addr 为 App 向量表起始地址。
 * 返回：正常情况下不返回；如果 App 返回则停在死循环。
 */
void boot_jump_to_app(uint32_t app_addr)
{
    uint32_t stack_addr;
    uint32_t entry_addr;
    void (*app_entry)(void);
    uint8_t index;

    stack_addr = *((const uint32_t *)app_addr);
    entry_addr = *((const uint32_t *)(app_addr + 4U));
    app_entry = (void (*)(void))entry_addr;

    /* 跳转前关闭 SysTick 和所有 NVIC 中断，防止 Bootloader 中断影响 App。 */
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (index = 0U; index < 8U; index++) {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    SCB->VTOR = app_addr;
    __set_MSP(stack_addr);
    __DSB();
    __ISB();
    __enable_irq();
    app_entry();

    while (1) {
    }
}
