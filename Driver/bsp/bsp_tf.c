#include "bsp_tf.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

#define BSP_TF_PATH "0:/log.txt"

static FATFS s_tf_fs;
static uint8_t s_sector0_test_buffer[512];
static uint8_t s_tf_mounted;
static bsp_tf_progress_t s_last_progress;
static bsp_tf_progress_callback_t s_progress_callback;
static uint32_t s_last_error;

/* 将 FatFs 的挂载错误转换成本项目统一的 TF 卡状态码。
 * 参数：result 为 FatFs 返回的 FRESULT。
 * 返回：bsp_tf_status_t，用于上层 OLED/Bootloader 显示和流程判断。
 */
static bsp_tf_status_t tf_mount_status_from_result(FRESULT result)
{
    switch (result) {
    case FR_NOT_READY:
        return BSP_TF_NOT_READY;

    case FR_DISK_ERR:
        return BSP_TF_DISK_FAIL;

    case FR_NO_FILESYSTEM:
        return BSP_TF_NO_FILESYSTEM;

    default:
        return BSP_TF_MOUNT_FAIL;
    }
}

/* 向固定缓冲区追加 1 个字符。
 * 参数：buffer 为目标缓冲区；index 为当前写入位置；size 为缓冲区大小；ch 为待追加字符。
 * 返回：1 表示追加成功，0 表示空间不足。
 */
static uint8_t append_char(char *buffer, uint16_t *index, uint16_t size, char ch)
{
    if (*index >= size) {
        return 0U;
    }

    buffer[*index] = ch;
    (*index)++;
    return 1U;
}

/* 向固定缓冲区追加 C 字符串。
 * 参数：buffer/index/size 描述目标缓冲区；str 为待追加字符串。
 * 返回：1 表示完整追加成功，0 表示空间不足。
 */
static uint8_t append_string(char *buffer, uint16_t *index, uint16_t size, const char *str)
{
    while ((str != 0) && (*str != '\0')) {
        if (append_char(buffer, index, size, *str) == 0U) {
            return 0U;
        }
        str++;
    }

    return 1U;
}

/* 将无符号整数按十进制追加到固定缓冲区。
 * 参数：buffer/index/size 描述目标缓冲区；value 为待输出整数。
 * 返回：1 表示追加成功，0 表示空间不足。
 */
static uint8_t append_uint(char *buffer, uint16_t *index, uint16_t size, uint32_t value)
{
    char digits[10];
    uint8_t digit_count;

    digit_count = 0U;
    do {
        digits[digit_count] = (char)('0' + (value % 10U));
        value /= 10U;
        digit_count++;
    } while ((value != 0U) && (digit_count < sizeof(digits)));

    while (digit_count > 0U) {
        digit_count--;
        if (append_char(buffer, index, size, digits[digit_count]) == 0U) {
            return 0U;
        }
    }

    return 1U;
}

/* 追加固定两位十进制数，用于生成 HH:MM:SS。
 * 参数：buffer/index/size 描述目标缓冲区；value 取值通常为 0~99。
 * 返回：1 表示追加成功，0 表示空间不足。
 */
static uint8_t append_two_digits(char *buffer, uint16_t *index, uint16_t size, uint8_t value)
{
    return append_char(buffer, index, size, (char)('0' + (value / 10U))) &&
           append_char(buffer, index, size, (char)('0' + (value % 10U)));
}

/* 生成一行 TF 卡日志文本。
 * 参数：buffer 为输出缓冲区；size 为缓冲区大小；second 为系统秒计数；datetime 为 RTC 时间。
 * 返回：生成的字节数，0 表示参数错误或缓冲区不足。
 */
static uint16_t build_log_line(char *buffer, uint16_t size, uint32_t second, const bsp_rtc_datetime_t *datetime)
{
    uint16_t index;

    index = 0U;
    if ((buffer == 0) || (datetime == 0)) {
        return 0U;
    }

    if (append_string(buffer, &index, size, "second=") == 0U ||
        append_uint(buffer, &index, size, second) == 0U ||
        append_string(buffer, &index, size, ", rtc=") == 0U ||
        append_two_digits(buffer, &index, size, datetime->hour) == 0U ||
        append_char(buffer, &index, size, ':') == 0U ||
        append_two_digits(buffer, &index, size, datetime->minute) == 0U ||
        append_char(buffer, &index, size, ':') == 0U ||
        append_two_digits(buffer, &index, size, datetime->second) == 0U ||
        append_string(buffer, &index, size, "\r\n") == 0U) {
        return 0U;
    }

    return index;
}

/* 初始化 TF 卡 BSP 的软件状态。
 * 参数：无。
 * 返回：无。
 */
void bsp_tf_init(void)
{
    s_tf_mounted = 0U;
    s_last_progress = BSP_TF_PROGRESS_MOUNT;
    s_progress_callback = 0;
    s_last_error = 0U;
}

/* 注册 TF 卡流程进度回调，供 Bootloader/OLED 显示当前阶段。
 * 参数：callback 为回调函数指针，传入 0 表示取消回调。
 * 返回：无。
 */
void bsp_tf_set_progress_callback(bsp_tf_progress_callback_t callback)
{
    s_progress_callback = callback;
}

/* 记录并上报 TF 卡流程进度。
 * 参数：progress 为当前阶段；value 为附加数值，例如扇区号或错误码。
 * 返回：无。
 */
void bsp_tf_emit_progress(bsp_tf_progress_t progress, uint32_t value)
{
    s_last_progress = progress;
    if (s_progress_callback != 0) {
        s_progress_callback(progress, value);
    }
}

/* 只记录 TF 卡进度，不触发回调。
 * 参数：progress 为当前阶段。
 * 返回：无。
 */
void bsp_tf_record_progress(bsp_tf_progress_t progress)
{
    s_last_progress = progress;
}

/* 获取最近一次 TF 卡流程进度。
 * 参数：无。
 * 返回：最近记录的 bsp_tf_progress_t。
 */
bsp_tf_progress_t bsp_tf_get_last_progress(void)
{
    return s_last_progress;
}

/* 保存最近一次 TF/SDIO 底层错误码。
 * 参数：error 为底层返回的错误值。
 * 返回：无。
 */
void bsp_tf_set_last_error(uint32_t error)
{
    s_last_error = error;
}

/* 获取最近一次 TF/SDIO 底层错误码。
 * 参数：无。
 * 返回：最近保存的错误值。
 */
uint32_t bsp_tf_get_last_error(void)
{
    return s_last_error;
}

/* 挂载 TF 卡文件系统。
 * 参数：无。
 * 返回：BSP_TF_OK 表示挂载成功，其余状态用于区分未就绪、磁盘错误或无文件系统。
 */
bsp_tf_status_t bsp_tf_mount(void)
{
    FRESULT result;

    if (s_tf_mounted != 0U) {
        return BSP_TF_OK;
    }

    /* f_mount 失败时不置位 mounted，方便后续重新插卡或再次尝试。 */
    bsp_tf_emit_progress(BSP_TF_PROGRESS_MOUNT, 0U);
    result = f_mount(&s_tf_fs, "0:", 1U);
    if (result != FR_OK) {
        return tf_mount_status_from_result(result);
    }

    s_tf_mounted = 1U;
    bsp_tf_emit_progress(BSP_TF_PROGRESS_READY, 0U);
    return BSP_TF_OK;
}

/* 读取 0 号扇区并检查 0x55AA 标志，用于快速验证 TF 卡底层读通路。
 * 参数：无。
 * 返回：BSP_TF_OK 表示扇区可读且签名正确，否则返回具体失败原因。
 */
bsp_tf_status_t bsp_tf_sector0_read_test(void)
{
    DSTATUS status;
    DRESULT result;

    status = disk_initialize(0U);
    if ((status & STA_NOINIT) != 0U) {
        return BSP_TF_NOT_READY;
    }

    bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_READ, 0U);
    result = disk_read(0U, s_sector0_test_buffer, 0U, 1U);
    if (result != RES_OK) {
        return BSP_TF_READ_FAIL;
    }

    if ((s_sector0_test_buffer[510] != 0x55U) || (s_sector0_test_buffer[511] != 0xAAU)) {
        return BSP_TF_SECTOR0_FAIL;
    }

    return BSP_TF_OK;
}

/* 追加写入一条 second + RTC 时间日志到 TF 卡。
 * 参数：second 为系统运行秒数；datetime 为当前 RTC 时间。
 * 返回：BSP_TF_OK 表示写入成功，其余状态表示挂载、打开或写入失败。
 */
bsp_tf_status_t bsp_tf_log_time(uint32_t second, const bsp_rtc_datetime_t *datetime)
{
    FIL file;
    FRESULT result;
    char line[64];
    uint16_t length;
    UINT written;
    bsp_tf_status_t mount_status;

    mount_status = bsp_tf_mount();
    if (mount_status != BSP_TF_OK) {
        return mount_status;
    }

    length = build_log_line(line, sizeof(line), second, datetime);
    if (length == 0U) {
        return BSP_TF_WRITE_FAIL;
    }

    /* 使用追加模式，避免每次按键写日志时覆盖历史记录。 */
    result = f_open(&file, BSP_TF_PATH, FA_OPEN_APPEND | FA_WRITE);
    if (result != FR_OK) {
        return BSP_TF_OPEN_FAIL;
    }

    result = f_write(&file, line, length, &written);
    if ((result != FR_OK) || (written != length)) {
        (void)f_close(&file);
        return BSP_TF_WRITE_FAIL;
    }

    result = f_close(&file);
    if (result != FR_OK) {
        return BSP_TF_WRITE_FAIL;
    }

    return BSP_TF_OK;
}
