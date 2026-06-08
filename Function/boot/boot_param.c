#include "boot_param.h"
#include "boot_flash.h"

#define BOOT_PARAM_LEGACY_TAIL_OFFSET 52U
#define BOOT_PARAM_LEGACY_V3_TAIL_OFFSET 68U
#define BOOT_PARAM_LEGACY_V4_TAIL_OFFSET 204U

/* 从内部 Flash 参数区读取 32 位值。
 * 参数：address 为绝对 Flash 地址。
 * 返回：该地址处的小端 32 位数据。
 */
static uint32_t boot_param_read_u32(uint32_t address)
{
    return *((const uint32_t *)address);
}

/* 填充一份默认系统参数。
 * 参数：param 为待初始化的参数结构体。
 * 返回：无；param 为 0 时直接返回。
 */
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
    param->ch2_threshold_bits = BOOT_PARAM_FLOAT_850_BITS;
    param->pt100_v_to_r_gain_bits = BOOT_PARAM_PT100_GAIN_BITS;
    param->pt100_v_to_r_offset_bits = BOOT_PARAM_PT100_OFFSET_BITS;
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
    param->dac_raw          = BOOT_DAC_RAW_DEFAULT;
    param->tail_magic       = BOOT_PARAM_TAIL_MAGIC;
}

/* 判断参数结构体是否为当前版本合法参数。
 * 参数：param 为待检查参数。
 * 返回：1 表示魔术字、版本和尾魔术字匹配，0 表示无效。
 */
uint8_t boot_param_is_valid(const boot_param_t *param)
{
    if (param == 0) {
        return 0U;
    }

    return (param->magic == BOOT_PARAM_MAGIC) &&
           (param->version == BOOT_PARAM_VERSION) &&
           (param->tail_magic == BOOT_PARAM_TAIL_MAGIC);
}

/* 兼容读取旧 v2 参数区。
 * 参数：param 为输出参数，会先填默认值再迁移旧字段。
 * 返回：1 表示旧参数识别成功，0 表示不是 v2 参数。
 */
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

/* 兼容读取旧 v3 参数区。
 * 参数：param 为输出参数，会补齐 v3 没有的默认字段。
 * 返回：1 表示旧参数识别成功，0 表示不是 v3 参数。
 */
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

/* 兼容读取旧 v4 参数区。
 * 参数：param 为输出参数，会补齐当前版本新增的 PT100 和 CH2 参数。
 * 返回：1 表示旧参数识别成功，0 表示不是 v4 参数。
 */
static uint8_t boot_param_load_legacy_v4(boot_param_t *param)
{
    const boot_param_t *stored;

    if (param == 0) {
        return 0U;
    }

    if ((boot_param_read_u32(BOOT_PARAM_ADDR) != BOOT_PARAM_MAGIC) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + 4U) != BOOT_PARAM_LEGACY_V4_VERSION) ||
        (boot_param_read_u32(BOOT_PARAM_ADDR + BOOT_PARAM_LEGACY_V4_TAIL_OFFSET) != BOOT_PARAM_TAIL_MAGIC)) {
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
    param->report_interval_code = stored->report_interval_code;
    param->alarm_report_mode = stored->alarm_report_mode;
    param->alarm_count = stored->alarm_count;
    for (uint8_t index = 0U; index < BOOT_ALARM_RECORD_MAX; index++) {
        /* 告警记录为固定长度数组，升级参数版本时逐项迁移。 */
        param->alarm_timestamp[index] = stored->alarm_timestamp[index];
        param->alarm_channel[index] = stored->alarm_channel[index];
        param->alarm_threshold_bits[index] = stored->alarm_threshold_bits[index];
        param->alarm_actual_bits[index] = stored->alarm_actual_bits[index];
    }
    param->dac_raw = stored->dac_raw;
    if (param->alarm_count > BOOT_ALARM_RECORD_MAX) {
        param->alarm_count = BOOT_ALARM_RECORD_MAX;
    }
    if (param->dac_raw > BOOT_DAC_RAW_MAX) {
        param->dac_raw = BOOT_DAC_RAW_DEFAULT;
    }
    return 1U;
}

/* 读取系统参数，必要时从旧版本参数迁移到内存结构。
 * 参数：param 为输出参数。
 * 返回：1 表示读到当前或旧版本有效参数，0 表示参数区无效并已输出默认值。
 */
uint8_t boot_param_load(boot_param_t *param)
{
    const boot_param_t *stored;

    if (param == 0) {
        return 0U;
    }

    stored = (const boot_param_t *)BOOT_PARAM_ADDR;
    *param = *stored;
    if (boot_param_is_valid(param) == 0U) {
        /* 依次尝试旧版本，保证之前烧录过的板子不用手动擦参数区。 */
        if (boot_param_load_legacy_v4(param) != 0U) {
            return 1U;
        }

        if (boot_param_load_legacy_v3(param) != 0U) {
            return 1U;
        }

        if (boot_param_load_legacy_v2(param) != 0U) {
            return 1U;
        }

        boot_param_default(param);
        return 0U;
    }

    if (param->dac_raw > BOOT_DAC_RAW_MAX) {
        param->dac_raw = BOOT_DAC_RAW_DEFAULT;
    }

    return 1U;
}

/* 擦除并写入系统参数区。
 * 参数：param 为待持久化参数。
 * 返回：1 表示写入成功，0 表示参数非法或 Flash 擦写失败。
 */
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

/* 标记已有外部 FLASH 待安装镜像。
 * 参数：slot_addr 为外部镜像槽地址；app_size/app_crc32/version 为镜像元数据。
 * 返回：1 表示参数写入成功，0 表示写入失败。
 */
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

/* 标记 App 收到 0501 后请求进入 Bootloader 串口升级窗口。
 * 参数：无。
 * 返回：1 表示参数写入成功，0 表示写入失败。
 */
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

/* 清除升级请求标志。
 * 参数：无。
 * 返回：1 表示清除成功或原本无请求，0 表示参数写入失败。
 */
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

/* 判断当前参数区是否存在串口升级请求。
 * 参数：无。
 * 返回：1 表示需要进入 Bootloader 串口升级窗口，0 表示不需要。
 */
uint8_t boot_param_is_usart_request(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return (param.update_flag == BOOT_UPDATE_FLAG_USART_REQUEST) ? 1U : 0U;
}

/* 记录 App 已成功安装。
 * 参数：header 为已安装镜像头。
 * 返回：1 表示参数写入成功，0 表示参数错误或写入失败。
 */
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

/* App 启动后确认自身可运行。
 * 参数：无。
 * 返回：1 表示确认成功，0 表示参数写入失败。
 */
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

/* 读取当前设备 ID。
 * 参数：无。
 * 返回：有效设备 ID；参数区为空或 0xFFFF 时返回默认 ID。
 */
uint16_t boot_param_get_device_id(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if ((param.device_id == 0U) || (param.device_id == 0xFFFFU)) {
        return BOOT_DEVICE_DEFAULT_ID;
    }

    return param.device_id;
}

/* 将波特率代码转换为实际波特率。
 * 参数：baudrate_code 为赛题协议中的波特率代码。
 * 返回：实际波特率，未知代码返回默认波特率。
 */
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

/* 读取当前波特率代码。
 * 参数：无。
 * 返回：有效波特率代码；参数区非法时返回默认代码。
 */
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

/* 设置并保存设备 ID。
 * 参数：device_id 为新的设备 ID，0 和 0xFFFF 非法。
 * 返回：1 表示保存成功，0 表示参数非法或写入失败。
 */
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

/* 设置并保存波特率代码。
 * 参数：baudrate_code 为新的波特率代码。
 * 返回：1 表示保存成功，0 表示代码非法或写入失败。
 */
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

/* 读取 CH0 变比的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_ch0_ratio_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch0_ratio_bits;
}

/* 读取 CH1 变比的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_ch1_ratio_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch1_ratio_bits;
}

/* 读取 CH0 阈值的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_ch0_threshold_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch0_threshold_bits;
}

/* 读取 CH1 阈值的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_ch1_threshold_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch1_threshold_bits;
}

/* 读取 CH2/PT100 阈值的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_ch2_threshold_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.ch2_threshold_bits;
}

/* 读取 PT100 电压转电阻增益的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_pt100_v_to_r_gain_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.pt100_v_to_r_gain_bits;
}

/* 读取 PT100 电压转电阻偏移的 IEEE754 bit 值。
 * 参数：无。
 * 返回：float 的 32 位原始 bit。
 */
uint32_t boot_param_get_pt100_v_to_r_offset_bits(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    return param.pt100_v_to_r_offset_bits;
}

/* 设置并保存 CH0 变比。
 * 参数：ratio_bits 为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_ch0_ratio_bits(uint32_t ratio_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch0_ratio_bits = ratio_bits;
    return boot_param_store(&param);
}

/* 设置并保存 CH1 变比。
 * 参数：ratio_bits 为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_ch1_ratio_bits(uint32_t ratio_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch1_ratio_bits = ratio_bits;
    return boot_param_store(&param);
}

/* 设置并保存 CH0 告警阈值。
 * 参数：threshold_bits 为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_ch0_threshold_bits(uint32_t threshold_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch0_threshold_bits = threshold_bits;
    return boot_param_store(&param);
}

/* 设置并保存 CH1 告警阈值。
 * 参数：threshold_bits 为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_ch1_threshold_bits(uint32_t threshold_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch1_threshold_bits = threshold_bits;
    return boot_param_store(&param);
}

/* 设置并保存 CH2/PT100 告警阈值。
 * 参数：threshold_bits 为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_ch2_threshold_bits(uint32_t threshold_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.ch2_threshold_bits = threshold_bits;
    return boot_param_store(&param);
}

/* 设置并保存 PT100 标定参数。
 * 参数：gain_bits/offset_bits 均为 IEEE754 float 原始 bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
uint8_t boot_param_set_pt100_calibration_bits(uint32_t gain_bits, uint32_t offset_bits)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    param.pt100_v_to_r_gain_bits = gain_bits;
    param.pt100_v_to_r_offset_bits = offset_bits;
    return boot_param_store(&param);
}

/* 读取自动上报间隔代码。
 * 参数：无。
 * 返回：有效间隔代码，非法时返回默认间隔。
 */
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

/* 设置并保存自动上报间隔代码。
 * 参数：interval_code 为 1s/3s/5s 对应代码。
 * 返回：1 表示保存成功，0 表示代码非法或写入失败。
 */
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

/* 读取告警上报模式。
 * 参数：无。
 * 返回：主动或被动告警模式，非法时返回默认模式。
 */
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

/* 设置并保存告警上报模式。
 * 参数：mode 为主动上报或被动查询模式。
 * 返回：1 表示保存成功，0 表示模式非法或写入失败。
 */
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

/* 读取 DAC 原始输出值。
 * 参数：无。
 * 返回：0~4095 的 DAC 原始码，非法时返回默认值。
 */
uint16_t boot_param_get_dac_raw(void)
{
    boot_param_t param;

    (void)boot_param_load(&param);
    if (param.dac_raw > BOOT_DAC_RAW_MAX) {
        return BOOT_DAC_RAW_DEFAULT;
    }

    return param.dac_raw;
}

/* 设置并保存 DAC 原始输出值。
 * 参数：dac_raw 为 0~4095 的输出码。
 * 返回：1 表示保存成功，0 表示参数非法或写入失败。
 */
uint8_t boot_param_set_dac_raw(uint16_t dac_raw)
{
    boot_param_t param;

    if (dac_raw > BOOT_DAC_RAW_MAX) {
        return 0U;
    }

    (void)boot_param_load(&param);
    param.dac_raw = dac_raw;
    return boot_param_store(&param);
}

/* 新增一条告警记录，最新记录放在第 0 项。
 * 参数：timestamp 为告警时间戳；channel 为通道号；threshold_bits/actual_bits 为阈值和实测值的 float bit。
 * 返回：1 表示保存成功，0 表示写入失败。
 */
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

    /* 固定容量数组采用向后搬移，保留最近 BOOT_ALARM_RECORD_MAX 条。 */
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

/* 清空所有告警记录。
 * 参数：无。
 * 返回：1 表示清空并保存成功，0 表示写入失败。
 */
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
