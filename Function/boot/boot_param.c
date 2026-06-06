#include "boot_param.h"
#include "boot_flash.h"

#define BOOT_PARAM_LEGACY_TAIL_OFFSET 52U
#define BOOT_PARAM_LEGACY_V3_TAIL_OFFSET 68U

static uint32_t boot_param_read_u32(uint32_t address)
{
    return *((const uint32_t *)address);
}

void boot_param_default(boot_param_t *param)
{
    if (param == 0) {
        return;
    }

    param->magic            = BOOT_PARAM_MAGIC;
    param->version          = BOOT_PARAM_VERSION;
    param->update_flag      = BOOT_UPDATE_FLAG_NONE;
    param->update_slot_addr = BOOT_EXT_SLOT0_ADDR;
    param->app_start_addr   = BOOT_APP_START_ADDR;
    param->app_size         = 0U;
    param->app_crc32        = 0xFFFFFFFFU;
    param->app_version      = 0U;
    param->confirm_magic    = BOOT_APP_CONFIRM_NONE;
    param->boot_count       = 0U;
    param->boot_fail_count  = 0U;
    param->last_error       = BOOT_OK;
    param->device_id        = BOOT_DEVICE_DEFAULT_ID;
    param->baudrate_code    = BOOT_USART_DEFAULT_BAUD_CODE;
    param->reserved0        = 0xFFU;
    param->ch0_ratio_bits   = BOOT_PARAM_FLOAT_1_BITS;
    param->ch1_ratio_bits   = BOOT_PARAM_FLOAT_1_BITS;
    param->ch0_threshold_bits = BOOT_PARAM_FLOAT_4095_BITS;
    param->ch1_threshold_bits = BOOT_PARAM_FLOAT_4095_BITS;
    param->report_interval_code = BOOT_REPORT_INTERVAL_DEFAULT;
    param->alarm_report_mode = BOOT_ALARM_MODE_DEFAULT;
    param->alarm_count = 0U;
    param->alarm_reserved0 = 0xFFU;
    for (uint8_t index = 0U; index < BOOT_ALARM_RECORD_MAX; index++) {
        param->alarm_timestamp[index] = 0U;
        param->alarm_channel[index] = 0U;
        param->alarm_threshold_bits[index] = 0U;
        param->alarm_actual_bits[index] = 0U;
    }
    param->alarm_reserved1[0] = 0xFFU;
    param->alarm_reserved1[1] = 0xFFU;
    param->tail_magic       = BOOT_PARAM_TAIL_MAGIC;
}

uint8_t boot_param_is_valid(const boot_param_t *param)
{
    if (param == 0) {
        return 0U;
    }

    return (param->magic == BOOT_PARAM_MAGIC) &&
           (param->version == BOOT_PARAM_VERSION) &&
           (param->tail_magic == BOOT_PARAM_TAIL_MAGIC);
}

static uint8_t boot_param_load_legacy_v2(boot_param_t *param)
{
    const boot_param_t *stored;

    if (param == 0) {
        return 0U;
    }

    if ((boot_param_read_u32(BOOT_PARAM_ADDR) != BOOT_PARAM_MAGIC) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + 4U) != BOOT_PARAM_LEGACY_VERSION) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + BOOT_PARAM_LEGACY_TAIL_OFFSET) != BOOT_PARAM_TAIL_MAGIC)) {
        return 0U;
    }

    stored = (const boot_param_t *)BOOT_PARAM_ADDR;
    boot_param_default(param);
    param->update_flag      = stored->update_flag;
    param->update_slot_addr = stored->update_slot_addr;
    param->app_start_addr   = stored->app_start_addr;
    param->app_size         = stored->app_size;
    param->app_crc32        = stored->app_crc32;
    param->app_version      = stored->app_version;
    param->confirm_magic    = stored->confirm_magic;
    param->boot_count       = stored->boot_count;
    param->boot_fail_count  = stored->boot_fail_count;
    param->last_error       = stored->last_error;
    param->device_id        = stored->device_id;
    param->baudrate_code    = stored->baudrate_code;
    param->reserved0        = stored->reserved0;
    return 1U;
}

static uint8_t boot_param_load_legacy_v3(boot_param_t *param)
{
    const boot_param_t *stored;

    if (param == 0) {
        return 0U;
    }

    if ((boot_param_read_u32(BOOT_PARAM_ADDR) != BOOT_PARAM_MAGIC) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + 4U) != BOOT_PARAM_LEGACY_V3_VERSION) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + BOOT_PARAM_LEGACY_V3_TAIL_OFFSET) != BOOT_PARAM_TAIL_MAGIC)) {
        return 0U;
    }

    stored = (const boot_param_t *)BOOT_PARAM_ADDR;
    boot_param_default(param);
    param->update_flag      = stored->update_flag;
    param->update_slot_addr = stored->update_slot_addr;
    param->app_start_addr   = stored->app_start_addr;
    param->app_size         = stored->app_size;
    param->app_crc32        = stored->app_crc32;
    param->app_version      = stored->app_version;
    param->confirm_magic    = stored->confirm_magic;
    param->boot_count       = stored->boot_count;
    param->boot_fail_count  = stored->boot_fail_count;
    param->last_error       = stored->last_error;
    param->device_id        = stored->device_id;
    param->baudrate_code    = stored->baudrate_code;
    param->reserved0        = stored->reserved0;
    param->ch0_ratio_bits   = stored->ch0_ratio_bits;
    param->ch1_ratio_bits   = stored->ch1_ratio_bits;
    param->ch0_threshold_bits = stored->ch0_threshold_bits;
    param->ch1_threshold_bits = stored->ch1_threshold_bits;
    return 1U;
}

uint8_t boot_param_load(boot_param_t *param)
{
    const boot_param_t *stored;

    if (param == 0) {
        return 0U;
    }

    stored = (const boot_param_t *)BOOT_PARAM_ADDR;
    *param = *stored;
    if (boot_param_is_valid(param) == 0U) {
        if (boot_param_load_legacy_v3(param) != 0U) {
            return 1U;
        }

        if (boot_param_load_legacy_v2(param) != 0U) {
            return 1U;
        }

        boot_param_default(param);
        return 0U;
    }

    return 1U;
}

uint8_t boot_param_store(const boot_param_t *param)
{
    if (boot_param_is_valid(param) == 0U) {
        return 0U;
    }

    if (boot_flash_erase(BOOT_PARAM_ADDR, BOOT_PARAM_SIZE) == 0U) {
        return 0U;
    }

    return boot_flash_write(BOOT_PARAM_ADDR, (const uint8_t *)param, sizeof(boot_param_t));
}

uint8_t boot_param_mark_pending(uint32_t slot_addr, uint32_t app_size, uint32_t app_crc32, uint32_t version)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.update_flag      = BOOT_UPDATE_FLAG_PENDING;
    param.update_slot_addr = slot_addr;
    param.app_start_addr   = BOOT_APP_START_ADDR;
    param.app_size         = app_size;
    param.app_crc32        = app_crc32;
    param.app_version      = version;
    param.confirm_magic    = BOOT_APP_CONFIRM_NONE;
    param.last_error       = BOOT_OK;

    return boot_param_store(&param);
}

uint8_t boot_param_mark_usart_request(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.update_flag      = BOOT_UPDATE_FLAG_USART_REQUEST;
    param.update_slot_addr = BOOT_FW_STAGING_ADDR;
    param.app_start_addr   = BOOT_APP_START_ADDR;
    param.confirm_magic    = BOOT_APP_CONFIRM_NONE;
    param.last_error       = BOOT_OK;

    return boot_param_store(&param);
}

uint8_t boot_param_clear_update_request(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if (param.update_flag == BOOT_UPDATE_FLAG_NONE) {
        return 1U;
    }

    param.update_flag = BOOT_UPDATE_FLAG_NONE;
    param.last_error  = BOOT_OK;

    return boot_param_store(&param);
}

uint8_t boot_param_is_usart_request(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return (param.update_flag == BOOT_UPDATE_FLAG_USART_REQUEST) ? 1U : 0U;
}

uint8_t boot_param_mark_app_installed(const boot_image_header_t *header)
{
    boot_param_t param;

    if (header == 0) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.update_flag      = BOOT_UPDATE_FLAG_NONE;
    param.update_slot_addr = BOOT_EXT_SLOT0_ADDR;
    param.app_start_addr   = header->target_addr;
    param.app_size         = header->image_size;
    param.app_crc32        = header->image_crc32;
    param.app_version      = header->image_version;
    param.confirm_magic    = BOOT_APP_CONFIRM_NONE;
    param.last_error       = BOOT_OK;

    return boot_param_store(&param);
}

uint8_t boot_param_confirm_app(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if ((param.update_flag == BOOT_UPDATE_FLAG_NONE) && (param.confirm_magic == BOOT_APP_CONFIRM_OK)) {
        return 1U;
    }

    param.update_flag   = BOOT_UPDATE_FLAG_NONE;
    param.confirm_magic = BOOT_APP_CONFIRM_OK;
    param.last_error    = BOOT_OK;

    return boot_param_store(&param);
}

uint16_t boot_param_get_device_id(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if ((param.device_id == 0U) || (param.device_id == 0xFFFFU)) {
        return BOOT_DEVICE_DEFAULT_ID;
    }

    return param.device_id;
}

uint32_t boot_param_baudrate_from_code(uint8_t baudrate_code)
{
    switch (baudrate_code) {
    case BOOT_USART_BAUD_CODE_4800:
        return 4800U;

    case BOOT_USART_BAUD_CODE_9600:
        return 9600U;

    case BOOT_USART_BAUD_CODE_19200:
        return 19200U;

    case BOOT_USART_BAUD_CODE_115200:
        return 115200U;

    default:
        return BOOT_USART_DEFAULT_BAUDRATE;
    }
}

uint8_t boot_param_get_baudrate_code(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    switch (param.baudrate_code) {
    case BOOT_USART_BAUD_CODE_4800:
    case BOOT_USART_BAUD_CODE_9600:
    case BOOT_USART_BAUD_CODE_19200:
    case BOOT_USART_BAUD_CODE_115200:
        return param.baudrate_code;

    default:
        return BOOT_USART_DEFAULT_BAUD_CODE;
    }
}

uint8_t boot_param_set_device_id(uint16_t device_id)
{
    boot_param_t param;

    if ((device_id == 0U) || (device_id == 0xFFFFU)) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.device_id = device_id;
    return boot_param_store(&param);
}

uint8_t boot_param_set_baudrate_code(uint8_t baudrate_code)
{
    boot_param_t param;

    if ((baudrate_code < BOOT_USART_BAUD_CODE_4800) ||
        (baudrate_code > BOOT_USART_BAUD_CODE_115200)) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.baudrate_code = baudrate_code;
    return boot_param_store(&param);
}

uint32_t boot_param_get_ch0_ratio_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch0_ratio_bits;
}

uint32_t boot_param_get_ch1_ratio_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch1_ratio_bits;
}

uint32_t boot_param_get_ch0_threshold_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch0_threshold_bits;
}

uint32_t boot_param_get_ch1_threshold_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch1_threshold_bits;
}

uint8_t boot_param_set_ch0_ratio_bits(uint32_t ratio_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch0_ratio_bits = ratio_bits;
    return boot_param_store(&param);
}

uint8_t boot_param_set_ch1_ratio_bits(uint32_t ratio_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch1_ratio_bits = ratio_bits;
    return boot_param_store(&param);
}

uint8_t boot_param_set_ch0_threshold_bits(uint32_t threshold_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch0_threshold_bits = threshold_bits;
    return boot_param_store(&param);
}

uint8_t boot_param_set_ch1_threshold_bits(uint32_t threshold_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch1_threshold_bits = threshold_bits;
    return boot_param_store(&param);
}

uint8_t boot_param_get_report_interval_code(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if ((param.report_interval_code < BOOT_REPORT_INTERVAL_1S) ||
        (param.report_interval_code > BOOT_REPORT_INTERVAL_5S)) {
        return BOOT_REPORT_INTERVAL_DEFAULT;
    }

    return param.report_interval_code;
}

uint8_t boot_param_set_report_interval_code(uint8_t interval_code)
{
    boot_param_t param;

    if ((interval_code < BOOT_REPORT_INTERVAL_1S) ||
        (interval_code > BOOT_REPORT_INTERVAL_5S)) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.report_interval_code = interval_code;
    return boot_param_store(&param);
}

uint8_t boot_param_get_alarm_report_mode(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if ((param.alarm_report_mode != BOOT_ALARM_MODE_ACTIVE) &&
        (param.alarm_report_mode != BOOT_ALARM_MODE_PASSIVE)) {
        return BOOT_ALARM_MODE_DEFAULT;
    }

    return param.alarm_report_mode;
}

uint8_t boot_param_set_alarm_report_mode(uint8_t mode)
{
    boot_param_t param;

    if ((mode != BOOT_ALARM_MODE_ACTIVE) &&
        (mode != BOOT_ALARM_MODE_PASSIVE)) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.alarm_report_mode = mode;
    return boot_param_store(&param);
}

uint8_t boot_param_add_alarm(uint32_t timestamp, uint8_t channel, uint32_t threshold_bits, uint32_t actual_bits)
{
    boot_param_t param;
    uint8_t index;
    uint8_t count;

    (void)boot_param_load(&param);
    count = param.alarm_count;
    if (count > BOOT_ALARM_RECORD_MAX) {
        count = BOOT_ALARM_RECORD_MAX;
    }

    if (count < BOOT_ALARM_RECORD_MAX) {
        count++;
    }

    for (index = (uint8_t)(count - 1U); index > 0U; index--) {
        param.alarm_timestamp[index] = param.alarm_timestamp[index - 1U];
        param.alarm_channel[index] = param.alarm_channel[index - 1U];
        param.alarm_threshold_bits[index] = param.alarm_threshold_bits[index - 1U];
        param.alarm_actual_bits[index] = param.alarm_actual_bits[index - 1U];
    }

    param.alarm_timestamp[0] = timestamp;
    param.alarm_channel[0] = channel;
    param.alarm_threshold_bits[0] = threshold_bits;
    param.alarm_actual_bits[0] = actual_bits;
    param.alarm_count = count;
    return boot_param_store(&param);
}

uint8_t boot_param_clear_alarm_records(void)
{
    boot_param_t param;
    uint8_t index;

    (void)boot_param_load(&param);
    param.alarm_count = 0U;
    for (index = 0U; index < BOOT_ALARM_RECORD_MAX; index++) {
        param.alarm_timestamp[index] = 0U;
        param.alarm_channel[index] = 0U;
        param.alarm_threshold_bits[index] = 0U;
        param.alarm_actual_bits[index] = 0U;
    }

    return boot_param_store(&param);
}
