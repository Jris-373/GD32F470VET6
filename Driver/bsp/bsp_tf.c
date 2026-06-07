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

static uint8_t append_char(char *buffer, uint16_t *index, uint16_t size, char ch)
{
    if (*index >= size) {
        return 0U;
    }

    buffer[*index] = ch;
    (*index)++;
    return 1U;
}

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

static uint8_t append_two_digits(char *buffer, uint16_t *index, uint16_t size, uint8_t value)
{
    return append_char(buffer, index, size, (char)('0' + (value / 10U))) &&
           append_char(buffer, index, size, (char)('0' + (value % 10U)));
}

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

void bsp_tf_init(void)
{
    s_tf_mounted = 0U;
    s_last_progress = BSP_TF_PROGRESS_MOUNT;
    s_progress_callback = 0;
    s_last_error = 0U;
}

void bsp_tf_set_progress_callback(bsp_tf_progress_callback_t callback)
{
    s_progress_callback = callback;
}

void bsp_tf_emit_progress(bsp_tf_progress_t progress, uint32_t value)
{
    s_last_progress = progress;
    if (s_progress_callback != 0) {
        s_progress_callback(progress, value);
    }
}

void bsp_tf_record_progress(bsp_tf_progress_t progress)
{
    s_last_progress = progress;
}

bsp_tf_progress_t bsp_tf_get_last_progress(void)
{
    return s_last_progress;
}

void bsp_tf_set_last_error(uint32_t error)
{
    s_last_error = error;
}

uint32_t bsp_tf_get_last_error(void)
{
    return s_last_error;
}

bsp_tf_status_t bsp_tf_mount(void)
{
    FRESULT result;

    if (s_tf_mounted != 0U) {
        return BSP_TF_OK;
    }

    bsp_tf_emit_progress(BSP_TF_PROGRESS_MOUNT, 0U);
    result = f_mount(&s_tf_fs, "0:", 1U);
    if (result != FR_OK) {
        return tf_mount_status_from_result(result);
    }

    s_tf_mounted = 1U;
    bsp_tf_emit_progress(BSP_TF_PROGRESS_READY, 0U);
    return BSP_TF_OK;
}

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
