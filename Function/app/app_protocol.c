#include "app_protocol.h"

#if defined(CMIS_BUILD_APP)

#include "app_version.h"
#include "boot_config.h"
#include "boot_param.h"
#include "bsp_adc.h"
#include "bsp_dac.h"
#include "bsp_led.h"
#include "bsp_oled.h"
#include "bsp_power.h"
#include "bsp_rtc.h"
#include "bsp_uart.h"
#include "gd32f4xx.h"
#include "protocol_ascii_hex.h"
#include "protocol_frame.h"
#include "systick.h"

#define APP_PROTOCOL_CMD_RESTART          0x0101U
#define APP_PROTOCOL_CMD_VERSION          0x0104U
#define APP_PROTOCOL_CMD_RTC_SET          0x0105U
#define APP_PROTOCOL_CMD_RTC_GET          0x0106U
#define APP_PROTOCOL_CMD_DEVICE_ID        0x0111U
#define APP_PROTOCOL_CMD_BAUDRATE         0x0112U
#define APP_PROTOCOL_CMD_SET_DEVICE_ID    0x01A1U
#define APP_PROTOCOL_CMD_SET_BAUDRATE     0x01A2U
#define APP_PROTOCOL_CMD_CH0_DATA         0x0201U
#define APP_PROTOCOL_CMD_CH1_DATA         0x0202U
#define APP_PROTOCOL_CMD_SET_CH0_RATIO    0x0241U
#define APP_PROTOCOL_CMD_SET_CH1_RATIO    0x0242U
#define APP_PROTOCOL_CMD_SET_REPORT_INTERVAL 0x0261U
#define APP_PROTOCOL_CMD_DAC_OUTPUT       0x0301U
#define APP_PROTOCOL_CMD_AUTO_REPORT_START 0x0302U
#define APP_PROTOCOL_CMD_AUTO_REPORT_STOP 0x0303U
#define APP_PROTOCOL_CMD_DEEPSLEEP        0x03AAU
#define APP_PROTOCOL_CMD_THRESHOLDS       0x0400U
#define APP_PROTOCOL_CMD_CH0_THRESHOLD    0x0401U
#define APP_PROTOCOL_CMD_CH1_THRESHOLD    0x0402U
#define APP_PROTOCOL_CMD_SET_CH0_THRESHOLD 0x0411U
#define APP_PROTOCOL_CMD_SET_CH1_THRESHOLD 0x0412U
#define APP_PROTOCOL_CMD_ENTER_BOOT       0x0501U
#define APP_PROTOCOL_CMD_ALARM_MODE       0x0601U
#define APP_PROTOCOL_CMD_ALARM_QUERY      0x0602U
#define APP_PROTOCOL_CMD_ALARM_CLEAR      0x0603U
#define APP_PROTOCOL_CMD_HOST_HEARTBEAT   0xFFFFU
#define APP_PROTOCOL_CMD_DEVICE_HEARTBEAT 0x8888U

#define APP_PROTOCOL_RESET_DELAY_MS       50U
#define APP_PROTOCOL_RS485_TURNAROUND_MS  5U
#define APP_PROTOCOL_MIN_ASCII_LENGTH     26U
#define APP_PROTOCOL_LENGTH_FIELD_END     16U
#define APP_PROTOCOL_YEAR_MIN             1970U
#define APP_PROTOCOL_YEAR_MAX             2099U
#define APP_PROTOCOL_ADC_TIMEOUT           100000U
#define APP_PROTOCOL_ALARM_TEXT_MAX       768U

typedef enum {
    APP_PROTOCOL_RX_NONE = 0,
    APP_PROTOCOL_RX_READY,
    APP_PROTOCOL_RX_ERROR,
} app_protocol_rx_status_t;

static char s_rx_ascii[PROTOCOL_FRAME_MAX_ASCII];
static uint16_t s_rx_length;
static uint16_t s_rx_expected_length;
static uint8_t s_rx_error_targets_local;
static uint16_t s_device_id;
static uint8_t s_baudrate_code;
static uint32_t s_ch0_ratio_bits;
static uint32_t s_ch1_ratio_bits;
static uint32_t s_ch0_threshold_bits;
static uint32_t s_ch1_threshold_bits;
static uint8_t s_report_interval_code;
static uint32_t s_report_interval_ms;
static uint8_t s_auto_report_enabled;
static uint32_t s_auto_report_last_tick;
static uint8_t s_alarm_report_mode;

static uint8_t app_protocol_is_leap_year(uint16_t year)
{
    if ((year % 400U) == 0U) {
        return 1U;
    }

    if ((year % 100U) == 0U) {
        return 0U;
    }

    return ((year % 4U) == 0U) ? 1U : 0U;
}

static uint8_t app_protocol_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_per_month[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U,
    };

    if ((month < 1U) || (month > 12U)) {
        return 0U;
    }

    if ((month == 2U) && (app_protocol_is_leap_year(year) != 0U)) {
        return 29U;
    }

    return days_per_month[month - 1U];
}

static uint8_t app_protocol_datetime_is_valid(const bsp_rtc_datetime_t *datetime)
{
    uint8_t month_days;

    if (datetime == 0) {
        return 0U;
    }

    if ((datetime->year < APP_PROTOCOL_YEAR_MIN) || (datetime->year > APP_PROTOCOL_YEAR_MAX) ||
        (datetime->month < 1U) || (datetime->month > 12U) ||
        (datetime->hour > 23U) || (datetime->minute > 59U) || (datetime->second > 59U)) {
        return 0U;
    }

    month_days = app_protocol_days_in_month(datetime->year, datetime->month);
    return ((datetime->date >= 1U) && (datetime->date <= month_days)) ? 1U : 0U;
}

static uint8_t app_protocol_unix_to_datetime(uint32_t timestamp, bsp_rtc_datetime_t *datetime)
{
    uint32_t days;
    uint32_t seconds;
    uint16_t year;
    uint8_t month;
    uint16_t year_days;
    uint8_t month_days;

    if (datetime == 0) {
        return 0U;
    }

    days    = timestamp / 86400U;
    seconds = timestamp % 86400U;
    year    = APP_PROTOCOL_YEAR_MIN;

    while (year <= APP_PROTOCOL_YEAR_MAX) {
        year_days = (app_protocol_is_leap_year(year) != 0U) ? 366U : 365U;
        if (days < year_days) {
            break;
        }

        days -= year_days;
        year++;
    }

    if (year > APP_PROTOCOL_YEAR_MAX) {
        return 0U;
    }

    month = 1U;
    while (month <= 12U) {
        month_days = app_protocol_days_in_month(year, month);
        if (days < month_days) {
            break;
        }

        days -= month_days;
        month++;
    }

    if (month > 12U) {
        return 0U;
    }

    datetime->year    = year;
    datetime->month   = month;
    datetime->date    = (uint8_t)(days + 1U);
    datetime->weekday = (uint8_t)(((timestamp / 86400U + 3U) % 7U) + 1U);
    datetime->hour    = (uint8_t)(seconds / 3600U);
    seconds %= 3600U;
    datetime->minute = (uint8_t)(seconds / 60U);
    datetime->second = (uint8_t)(seconds % 60U);

    return 1U;
}

static uint8_t app_protocol_datetime_to_unix(const bsp_rtc_datetime_t *datetime, uint32_t *timestamp)
{
    uint32_t days;
    uint16_t year;
    uint8_t month;

    if ((timestamp == 0) || (app_protocol_datetime_is_valid(datetime) == 0U)) {
        return 0U;
    }

    days = 0U;
    for (year = APP_PROTOCOL_YEAR_MIN; year < datetime->year; year++) {
        days += (app_protocol_is_leap_year(year) != 0U) ? 366U : 365U;
    }

    for (month = 1U; month < datetime->month; month++) {
        days += app_protocol_days_in_month(datetime->year, month);
    }

    days += (uint32_t)(datetime->date - 1U);
    *timestamp = days * 86400U + (uint32_t)datetime->hour * 3600U +
                 (uint32_t)datetime->minute * 60U + datetime->second;

    return 1U;
}

static uint32_t app_protocol_read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24U) |
           ((uint32_t)data[1] << 16U) |
           ((uint32_t)data[2] << 8U) |
           (uint32_t)data[3];
}

static uint16_t app_protocol_read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void app_protocol_write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
}

static void app_protocol_write_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static void app_protocol_write_float_be(uint8_t *data, float value)
{
    union {
        float floating;
        uint32_t integer;
    } conversion;

    conversion.floating = value;
    app_protocol_write_be32(data, conversion.integer);
}

static float app_protocol_float_from_bits(uint32_t bits)
{
    union {
        float floating;
        uint32_t integer;
    } conversion;

    conversion.integer = bits;
    return conversion.floating;
}

static uint32_t app_protocol_float_to_bits(float value)
{
    union {
        float floating;
        uint32_t integer;
    } conversion;

    conversion.floating = value;
    return conversion.integer;
}

static uint8_t app_protocol_float_bits_are_valid(uint32_t bits)
{
    return ((bits & 0x7F800000U) == 0x7F800000U) ? 0U : 1U;
}

static void app_protocol_receiver_reset(void)
{
    s_rx_length          = 0U;
    s_rx_expected_length = 0U;
}

static uint8_t app_protocol_start_char_matches(uint16_t offset, char ch)
{
    switch (offset) {
    case 0U:
        return ((ch == 'A') || (ch == 'a')) ? 1U : 0U;

    case 1U:
        return (ch == '5') ? 1U : 0U;

    case 2U:
        return ((ch == 'B') || (ch == 'b')) ? 1U : 0U;

    case 3U:
        return (ch == '6') ? 1U : 0U;

    default:
        return 0U;
    }
}

static uint8_t app_protocol_decode_rx_device_id(uint16_t *device_id)
{
    uint8_t nibble;
    uint16_t value;
    uint16_t index;

    if ((device_id == 0) || (s_rx_length < 8U)) {
        return 0U;
    }

    value = 0U;
    for (index = 4U; index < 8U; index++) {
        if (protocol_hex_char_to_nibble(s_rx_ascii[index], &nibble) == 0U) {
            return 0U;
        }

        value = (uint16_t)((value << 4U) | nibble);
    }

    *device_id = value;
    return 1U;
}

static uint8_t app_protocol_rx_targets_local_device(void)
{
    uint16_t device_id;

    if (app_protocol_decode_rx_device_id(&device_id) == 0U) {
        return 0U;
    }

    return ((device_id == s_device_id) || (device_id == PROTOCOL_DEVICE_BROADCAST)) ? 1U : 0U;
}

static app_protocol_rx_status_t app_protocol_receive_error(void)
{
    s_rx_error_targets_local = app_protocol_rx_targets_local_device();
    app_protocol_receiver_reset();
    return APP_PROTOCOL_RX_ERROR;
}

static uint8_t app_protocol_set_expected_length(void)
{
    uint8_t high;
    uint8_t low;
    uint16_t payload_length;

    if (s_rx_length != APP_PROTOCOL_LENGTH_FIELD_END) {
        return 0U;
    }

    if ((protocol_hex_char_to_nibble(s_rx_ascii[14], &high) == 0U) ||
        (protocol_hex_char_to_nibble(s_rx_ascii[15], &low) == 0U)) {
        return 0U;
    }

    payload_length      = (uint16_t)((high << 4U) | low);
    s_rx_expected_length = (uint16_t)(APP_PROTOCOL_MIN_ASCII_LENGTH + payload_length * 2U);
    return (s_rx_expected_length <= PROTOCOL_FRAME_MAX_ASCII) ? 1U : 0U;
}

static app_protocol_rx_status_t app_protocol_receive_byte(uint8_t data, protocol_frame_t *frame)
{
    protocol_status_t status;
    uint8_t nibble;

    if (frame == 0) {
        return APP_PROTOCOL_RX_ERROR;
    }

    if ((data == '\r') || (data == '\n')) {
        if (s_rx_length == 0U) {
            return APP_PROTOCOL_RX_NONE;
        }

        return app_protocol_receive_error();
    }

    if (protocol_hex_char_to_nibble((char)data, &nibble) == 0U) {
        if (s_rx_length == 0U) {
            return APP_PROTOCOL_RX_NONE;
        }

        return app_protocol_receive_error();
    }

    if (s_rx_length < 4U) {
        if (app_protocol_start_char_matches(s_rx_length, (char)data) == 0U) {
            app_protocol_receiver_reset();
            if (app_protocol_start_char_matches(0U, (char)data) != 0U) {
                s_rx_ascii[0] = (char)data;
                s_rx_length   = 1U;
            }
            return APP_PROTOCOL_RX_NONE;
        }
    }

    if (s_rx_length >= PROTOCOL_FRAME_MAX_ASCII) {
        return app_protocol_receive_error();
    }

    s_rx_ascii[s_rx_length] = (char)data;
    s_rx_length++;

    if ((s_rx_length == APP_PROTOCOL_LENGTH_FIELD_END) && (app_protocol_set_expected_length() == 0U)) {
        return app_protocol_receive_error();
    }

    if ((s_rx_expected_length != 0U) && (s_rx_length == s_rx_expected_length)) {
        status = protocol_frame_parse_ascii(s_rx_ascii, s_rx_length, frame);
        if (status != PROTOCOL_STATUS_OK) {
            return app_protocol_receive_error();
        }

        app_protocol_receiver_reset();
        return APP_PROTOCOL_RX_READY;
    }

    return APP_PROTOCOL_RX_NONE;
}

static void app_protocol_send_frame(uint8_t type, uint16_t command, const uint8_t *payload, uint8_t length)
{
    protocol_frame_t frame;
    char ascii[PROTOCOL_FRAME_MAX_ASCII + 2U];
    uint16_t ascii_length;
    uint16_t index;

    frame.device_id = s_device_id;
    frame.type       = type;
    frame.command    = command;
    frame.length     = length;

    for (index = 0U; index < length; index++) {
        frame.payload[index] = payload[index];
    }

    if (protocol_frame_build_ascii(&frame, ascii, PROTOCOL_FRAME_MAX_ASCII, &ascii_length) != PROTOCOL_STATUS_OK) {
        return;
    }

    ascii[ascii_length++] = '\r';
    ascii[ascii_length++] = '\n';

    delay_1ms(APP_PROTOCOL_RS485_TURNAROUND_MS);
    bsp_uart1_rs485_send_buffer((const uint8_t *)ascii, ascii_length);
}

static void app_protocol_send_ok(uint16_t command)
{
    uint8_t payload = 0xFFU;

    app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, command, &payload, 1U);
}

static void app_protocol_send_error(uint16_t command)
{
    app_protocol_send_frame(PROTOCOL_TYPE_ERROR, command, 0, 0U);
}

static void app_protocol_send_text(const char *text, uint16_t length)
{
    if ((text == 0) || (length == 0U)) {
        return;
    }

    delay_1ms(APP_PROTOCOL_RS485_TURNAROUND_MS);
    bsp_uart1_rs485_send_buffer((const uint8_t *)text, length);
}

static void app_protocol_append_char(char *buffer, uint16_t *length, uint16_t size, char ch)
{
    if ((buffer == 0) || (length == 0) || (*length >= size)) {
        return;
    }

    buffer[*length] = ch;
    (*length)++;
}

static void app_protocol_append_text(char *buffer, uint16_t *length, uint16_t size, const char *text)
{
    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        app_protocol_append_char(buffer, length, size, *text);
        text++;
    }
}

static void app_protocol_append_uint(char *buffer, uint16_t *length, uint16_t size, uint32_t value)
{
    char digits[10];
    uint8_t count;

    count = 0U;
    do {
        digits[count] = (char)('0' + (value % 10U));
        value /= 10U;
        count++;
    } while ((value != 0U) && (count < sizeof(digits)));

    while (count > 0U) {
        count--;
        app_protocol_append_char(buffer, length, size, digits[count]);
    }
}

static void app_protocol_append_padded_uint(char *buffer, uint16_t *length, uint16_t size, uint32_t value, uint8_t width)
{
    uint32_t divisor;

    divisor = 1U;
    while (width > 1U) {
        divisor *= 10U;
        width--;
    }

    while (divisor > 0U) {
        app_protocol_append_char(buffer, length, size, (char)('0' + ((value / divisor) % 10U)));
        divisor /= 10U;
    }
}

static void app_protocol_append_float_2(char *buffer, uint16_t *length, uint16_t size, float value)
{
    uint32_t scaled;

    if (value < 0.0f) {
        app_protocol_append_char(buffer, length, size, '-');
        value = -value;
    }

    scaled = (uint32_t)(value * 100.0f + 0.5f);
    app_protocol_append_uint(buffer, length, size, scaled / 100U);
    app_protocol_append_char(buffer, length, size, '.');
    app_protocol_append_padded_uint(buffer, length, size, scaled % 100U, 2U);
}

static void app_protocol_append_datetime(char *buffer, uint16_t *length, uint16_t size, uint32_t timestamp)
{
    bsp_rtc_datetime_t datetime;

    if (app_protocol_unix_to_datetime(timestamp, &datetime) == 0U) {
        app_protocol_append_text(buffer, length, size, "1970-01-01 00:00:00");
        return;
    }

    app_protocol_append_padded_uint(buffer, length, size, datetime.year, 4U);
    app_protocol_append_char(buffer, length, size, '-');
    app_protocol_append_padded_uint(buffer, length, size, datetime.month, 2U);
    app_protocol_append_char(buffer, length, size, '-');
    app_protocol_append_padded_uint(buffer, length, size, datetime.date, 2U);
    app_protocol_append_char(buffer, length, size, ' ');
    app_protocol_append_padded_uint(buffer, length, size, datetime.hour, 2U);
    app_protocol_append_char(buffer, length, size, ':');
    app_protocol_append_padded_uint(buffer, length, size, datetime.minute, 2U);
    app_protocol_append_char(buffer, length, size, ':');
    app_protocol_append_padded_uint(buffer, length, size, datetime.second, 2U);
}

static void app_protocol_append_alarm_line(char *buffer,
                                           uint16_t *length,
                                           uint16_t size,
                                           uint32_t timestamp,
                                           uint8_t channel,
                                           uint32_t threshold_bits,
                                           uint32_t actual_bits)
{
    app_protocol_append_datetime(buffer, length, size, timestamp);
    app_protocol_append_text(buffer, length, size, " | CH");
    app_protocol_append_char(buffer, length, size, (channel == 0U) ? '0' : '1');
    app_protocol_append_text(buffer, length, size, " | ");
    app_protocol_append_float_2(buffer, length, size, app_protocol_float_from_bits(threshold_bits));
    app_protocol_append_text(buffer, length, size, " | ");
    app_protocol_append_float_2(buffer, length, size, app_protocol_float_from_bits(actual_bits));
    app_protocol_append_text(buffer, length, size, "\r\n");
}

static uint32_t app_protocol_report_interval_ms_from_code(uint8_t interval_code)
{
    switch (interval_code) {
    case BOOT_REPORT_INTERVAL_3S:
        return 3000U;

    case BOOT_REPORT_INTERVAL_5S:
        return 5000U;

    case BOOT_REPORT_INTERVAL_1S:
    default:
        return 1000U;
    }
}

static uint8_t app_protocol_current_timestamp(uint32_t *timestamp)
{
    bsp_rtc_datetime_t datetime;

    if (timestamp == 0) {
        return 0U;
    }

    bsp_rtc_get_datetime(&datetime);
    return app_protocol_datetime_to_unix(&datetime, timestamp);
}

static uint8_t app_protocol_sample_adc_value(bsp_adc_input_t input, float *value)
{
    uint16_t raw;
    float ratio;

    if (value == 0) {
        return 0U;
    }

    if (bsp_adc_read_input_raw_timeout(input, &raw, APP_PROTOCOL_ADC_TIMEOUT) == 0U) {
        return 0U;
    }

    if (input == BSP_ADC_INPUT_CH0) {
        ratio = app_protocol_float_from_bits(s_ch0_ratio_bits);
    } else {
        ratio = app_protocol_float_from_bits(s_ch1_ratio_bits);
    }

    *value = (float)raw * ratio;
    return 1U;
}

static void app_protocol_check_alarm(uint32_t timestamp, uint8_t channel, uint32_t threshold_bits, float actual)
{
    char text[96];
    uint16_t length;
    uint32_t actual_bits;
    float threshold;

    if (app_protocol_float_bits_are_valid(threshold_bits) == 0U) {
        return;
    }

    threshold = app_protocol_float_from_bits(threshold_bits);
    if (actual <= threshold) {
        return;
    }

    actual_bits = app_protocol_float_to_bits(actual);
    if (boot_param_add_alarm(timestamp, channel, threshold_bits, actual_bits) == 0U) {
        return;
    }

    if (s_alarm_report_mode != BOOT_ALARM_MODE_ACTIVE) {
        return;
    }

    length = 0U;
    app_protocol_append_alarm_line(text, &length, sizeof(text), timestamp, channel, threshold_bits, actual_bits);
    app_protocol_send_text(text, length);
}

static uint8_t app_protocol_sample_report_values(uint32_t *timestamp, float *ch0_value, float *ch1_value)
{
    if ((timestamp == 0) || (ch0_value == 0) || (ch1_value == 0)) {
        return 0U;
    }

    if (app_protocol_current_timestamp(timestamp) == 0U) {
        return 0U;
    }

    if (app_protocol_sample_adc_value(BSP_ADC_INPUT_CH0, ch0_value) == 0U) {
        return 0U;
    }

    if (app_protocol_sample_adc_value(BSP_ADC_INPUT_CH1, ch1_value) == 0U) {
        return 0U;
    }

    app_protocol_check_alarm(*timestamp, 0U, s_ch0_threshold_bits, *ch0_value);
    app_protocol_check_alarm(*timestamp, 1U, s_ch1_threshold_bits, *ch1_value);
    return 1U;
}

static uint8_t app_protocol_send_report_data(uint8_t send_error_on_failure)
{
    uint8_t payload[12];
    uint32_t timestamp;
    float ch0_value;
    float ch1_value;

    if (app_protocol_sample_report_values(&timestamp, &ch0_value, &ch1_value) == 0U) {
        if (send_error_on_failure != 0U) {
            app_protocol_send_error(APP_PROTOCOL_CMD_AUTO_REPORT_START);
        }
        return 0U;
    }

    app_protocol_write_be32(payload, timestamp);
    app_protocol_write_float_be(&payload[4], ch0_value);
    app_protocol_write_float_be(&payload[8], ch1_value);
    app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, APP_PROTOCOL_CMD_AUTO_REPORT_START, payload, sizeof(payload));
    return 1U;
}

static void app_protocol_handle_rtc_set(const protocol_frame_t *frame)
{
    bsp_rtc_datetime_t datetime;
    uint32_t timestamp;

    timestamp = app_protocol_read_be32(frame->payload);
    if ((app_protocol_unix_to_datetime(timestamp, &datetime) != 0U) &&
        (bsp_rtc_set_datetime(&datetime) != 0U)) {
        app_protocol_send_ok(frame->command);
        return;
    }

    app_protocol_send_error(frame->command);
}

static void app_protocol_handle_rtc_get(const protocol_frame_t *frame)
{
    bsp_rtc_datetime_t datetime;
    uint8_t payload[4];
    uint32_t timestamp;

    bsp_rtc_get_datetime(&datetime);
    if (app_protocol_datetime_to_unix(&datetime, &timestamp) == 0U) {
        app_protocol_send_error(frame->command);
        return;
    }

    app_protocol_write_be32(payload, timestamp);
    app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, sizeof(payload));
}

static void app_protocol_handle_adc_get(const protocol_frame_t *frame, bsp_adc_input_t input)
{
    uint8_t payload[4];
    uint32_t timestamp;
    uint32_t threshold_bits;
    uint8_t channel;
    float value;

    if (app_protocol_sample_adc_value(input, &value) == 0U) {
        app_protocol_send_error(frame->command);
        return;
    }

    if (input == BSP_ADC_INPUT_CH0) {
        channel        = 0U;
        threshold_bits = s_ch0_threshold_bits;
    } else {
        channel        = 1U;
        threshold_bits = s_ch1_threshold_bits;
    }

    if (app_protocol_current_timestamp(&timestamp) != 0U) {
        app_protocol_check_alarm(timestamp, channel, threshold_bits, value);
    }

    app_protocol_write_float_be(payload, value);
    app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, sizeof(payload));
}

static void app_protocol_handle_ratio_set(const protocol_frame_t *frame, uint8_t channel)
{
    uint32_t ratio_bits;
    uint8_t ok;

    ratio_bits = app_protocol_read_be32(frame->payload);
    if (app_protocol_float_bits_are_valid(ratio_bits) == 0U) {
        app_protocol_send_error(frame->command);
        return;
    }

    if (channel == 0U) {
        ok = boot_param_set_ch0_ratio_bits(ratio_bits);
        if (ok != 0U) {
            s_ch0_ratio_bits = ratio_bits;
        }
    } else {
        ok = boot_param_set_ch1_ratio_bits(ratio_bits);
        if (ok != 0U) {
            s_ch1_ratio_bits = ratio_bits;
        }
    }

    if (ok != 0U) {
        app_protocol_send_ok(frame->command);
    } else {
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_handle_threshold_get(const protocol_frame_t *frame, uint8_t channel)
{
    uint8_t payload[8];

    if (channel == 0U) {
        app_protocol_write_be32(payload, s_ch0_threshold_bits);
        app_protocol_write_be32(&payload[4], s_ch1_threshold_bits);
        app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, 8U);
        return;
    }

    if (channel == 1U) {
        app_protocol_write_be32(payload, s_ch0_threshold_bits);
    } else {
        app_protocol_write_be32(payload, s_ch1_threshold_bits);
    }

    app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, 4U);
}

static void app_protocol_handle_threshold_set(const protocol_frame_t *frame, uint8_t channel)
{
    uint32_t threshold_bits;
    uint8_t ok;

    threshold_bits = app_protocol_read_be32(frame->payload);
    if (app_protocol_float_bits_are_valid(threshold_bits) == 0U) {
        app_protocol_send_error(frame->command);
        return;
    }

    if (channel == 1U) {
        ok = boot_param_set_ch0_threshold_bits(threshold_bits);
        if (ok != 0U) {
            s_ch0_threshold_bits = threshold_bits;
        }
    } else {
        ok = boot_param_set_ch1_threshold_bits(threshold_bits);
        if (ok != 0U) {
            s_ch1_threshold_bits = threshold_bits;
        }
    }

    if (ok != 0U) {
        app_protocol_send_ok(frame->command);
    } else {
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_handle_report_interval_set(const protocol_frame_t *frame)
{
    if (boot_param_set_report_interval_code(frame->payload[0]) != 0U) {
        s_report_interval_code = frame->payload[0];
        s_report_interval_ms   = app_protocol_report_interval_ms_from_code(s_report_interval_code);
        app_protocol_send_ok(frame->command);
    } else {
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_handle_alarm_mode_set(const protocol_frame_t *frame)
{
    if (boot_param_set_alarm_report_mode(frame->payload[0]) != 0U) {
        s_alarm_report_mode = frame->payload[0];
        app_protocol_send_ok(frame->command);
    } else {
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_handle_alarm_query(void)
{
    boot_param_t param;
    char text[APP_PROTOCOL_ALARM_TEXT_MAX];
    uint16_t length;
    uint8_t index;
    uint8_t count;

    (void)boot_param_load(&param);
    count = param.alarm_count;
    if (count > BOOT_ALARM_RECORD_MAX) {
        count = BOOT_ALARM_RECORD_MAX;
    }

    length = 0U;
    if (count == 0U) {
        app_protocol_append_text(text, &length, sizeof(text), "empty\r\n");
    } else {
        for (index = 0U; index < count; index++) {
            app_protocol_append_alarm_line(text,
                                           &length,
                                           sizeof(text),
                                           param.alarm_timestamp[index],
                                           param.alarm_channel[index],
                                           param.alarm_threshold_bits[index],
                                           param.alarm_actual_bits[index]);
        }
    }

    app_protocol_send_text(text, length);
}

static void app_protocol_handle_alarm_clear(const protocol_frame_t *frame)
{
    if (boot_param_clear_alarm_records() != 0U) {
        app_protocol_send_ok(frame->command);
    } else {
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_restore_after_deepsleep(void)
{
    bsp_led_init();
    bsp_oled_init();
    bsp_oled_display_on();
    bsp_oled_clear();
    bsp_oled_show_string(0U, 0U, "APP VERSION");
    bsp_oled_show_string(0U, 16U, "VER:");
    bsp_oled_show_string(32U, 16U, APP_VERSION_TEXT);
    bsp_uart1_rs485_init(boot_param_baudrate_from_code(s_baudrate_code));
}

static void app_protocol_handle_deepsleep(const protocol_frame_t *frame)
{
    static const char wakeup_text[] = "instrument wakeup";

    s_auto_report_enabled = 0U;
    app_protocol_send_ok(frame->command);
    delay_1ms(20U);

    bsp_oled_display_off();
    bsp_led_off();

    if (bsp_power_deepsleep_rtc_alarm(10U) != 0U) {
        app_protocol_restore_after_deepsleep();
        delay_1ms(20U);
        app_protocol_send_text(wakeup_text, (uint16_t)(sizeof(wakeup_text) - 1U));
    } else {
        app_protocol_restore_after_deepsleep();
        app_protocol_send_error(frame->command);
    }
}

static void app_protocol_handle_command(const protocol_frame_t *frame)
{
    uint8_t payload[4];

    if ((frame->command == APP_PROTOCOL_CMD_RESTART) && (frame->length == 0U)) {
        app_protocol_send_ok(frame->command);
        delay_1ms(APP_PROTOCOL_RESET_DELAY_MS);
        NVIC_SystemReset();
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_VERSION) && (frame->length == 0U)) {
        payload[0] = APP_VERSION_MAJOR;
        payload[1] = APP_VERSION_MINOR;
        payload[2] = APP_VERSION_PATCH;
        payload[3] = APP_VERSION_BUILD;
        app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, 4U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_RTC_SET) && (frame->length == 4U)) {
        app_protocol_handle_rtc_set(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_RTC_GET) && (frame->length == 0U)) {
        app_protocol_handle_rtc_get(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_DEVICE_ID) && (frame->length == 0U)) {
        app_protocol_write_be16(payload, s_device_id);
        app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, 2U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_BAUDRATE) && (frame->length == 0U)) {
        payload[0] = s_baudrate_code;
        app_protocol_send_frame(PROTOCOL_TYPE_RESPONSE, frame->command, payload, 1U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_CH0_DATA) && (frame->length == 0U)) {
        app_protocol_handle_adc_get(frame, BSP_ADC_INPUT_CH0);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_CH1_DATA) && (frame->length == 0U)) {
        app_protocol_handle_adc_get(frame, BSP_ADC_INPUT_CH1);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_REPORT_INTERVAL) && (frame->length == 1U)) {
        app_protocol_handle_report_interval_set(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_DAC_OUTPUT) && (frame->length == 2U)) {
        uint16_t raw;

        raw = app_protocol_read_be16(frame->payload);
        if (raw <= BSP_DAC_MAX_RAW) {
            bsp_dac_write_raw(raw);
            app_protocol_send_ok(frame->command);
        } else {
            app_protocol_send_error(frame->command);
        }
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_AUTO_REPORT_START) && (frame->length == 0U)) {
        s_auto_report_enabled   = 1U;
        s_auto_report_last_tick = systick_get_tick();
        if (app_protocol_send_report_data(1U) == 0U) {
            s_auto_report_enabled = 0U;
        }
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_AUTO_REPORT_STOP) && (frame->length == 0U)) {
        s_auto_report_enabled = 0U;
        app_protocol_send_ok(frame->command);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_DEEPSLEEP) && (frame->length == 0U)) {
        app_protocol_handle_deepsleep(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_CH0_RATIO) && (frame->length == 4U)) {
        app_protocol_handle_ratio_set(frame, 0U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_CH1_RATIO) && (frame->length == 4U)) {
        app_protocol_handle_ratio_set(frame, 1U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_THRESHOLDS) && (frame->length == 0U)) {
        app_protocol_handle_threshold_get(frame, 0U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_CH0_THRESHOLD) && (frame->length == 0U)) {
        app_protocol_handle_threshold_get(frame, 1U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_CH1_THRESHOLD) && (frame->length == 0U)) {
        app_protocol_handle_threshold_get(frame, 2U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_CH0_THRESHOLD) && (frame->length == 4U)) {
        app_protocol_handle_threshold_set(frame, 1U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_CH1_THRESHOLD) && (frame->length == 4U)) {
        app_protocol_handle_threshold_set(frame, 2U);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_DEVICE_ID) && (frame->length == 2U)) {
        uint16_t device_id;

        device_id = app_protocol_read_be16(frame->payload);
        if (boot_param_set_device_id(device_id) != 0U) {
            s_device_id = device_id;
            app_protocol_send_ok(frame->command);
        } else {
            app_protocol_send_error(frame->command);
        }
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_SET_BAUDRATE) && (frame->length == 1U)) {
        if (boot_param_set_baudrate_code(frame->payload[0]) != 0U) {
            s_baudrate_code = frame->payload[0];
            app_protocol_send_ok(frame->command);
            delay_1ms(APP_PROTOCOL_RESET_DELAY_MS);
            NVIC_SystemReset();
        } else {
            app_protocol_send_error(frame->command);
        }
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_ALARM_MODE) && (frame->length == 1U)) {
        app_protocol_handle_alarm_mode_set(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_ALARM_QUERY) && (frame->length == 0U)) {
        app_protocol_handle_alarm_query();
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_ALARM_CLEAR) && (frame->length == 0U)) {
        app_protocol_handle_alarm_clear(frame);
        return;
    }

    if ((frame->command == APP_PROTOCOL_CMD_ENTER_BOOT) && (frame->length == 0U)) {
        if (boot_param_mark_usart_request() != 0U) {
            app_protocol_send_ok(frame->command);
            delay_1ms(APP_PROTOCOL_RESET_DELAY_MS);
            NVIC_SystemReset();
        } else {
            app_protocol_send_error(frame->command);
        }
        return;
    }

    app_protocol_send_error(PROTOCOL_CMD_ERROR);
}

static void app_protocol_handle_frame(const protocol_frame_t *frame)
{
    if ((frame->device_id != s_device_id) && (frame->device_id != PROTOCOL_DEVICE_BROADCAST)) {
        return;
    }

    if (s_auto_report_enabled != 0U) {
        if ((frame->type == PROTOCOL_TYPE_COMMAND) &&
            ((frame->command == APP_PROTOCOL_CMD_AUTO_REPORT_STOP) ||
             (frame->command == APP_PROTOCOL_CMD_DEEPSLEEP)) &&
            (frame->length == 0U)) {
            app_protocol_handle_command(frame);
        }
        return;
    }

    if (frame->device_id == PROTOCOL_DEVICE_BROADCAST) {
        if ((frame->type == PROTOCOL_TYPE_HEARTBEAT) &&
            (frame->command == APP_PROTOCOL_CMD_HOST_HEARTBEAT) &&
            (frame->length == 0U)) {
            app_protocol_send_heartbeat();
            return;
        }

        if ((frame->type == PROTOCOL_TYPE_COMMAND) &&
            (frame->command == APP_PROTOCOL_CMD_DEVICE_ID) &&
            (frame->length == 0U)) {
            app_protocol_handle_command(frame);
            return;
        }

        app_protocol_send_error(PROTOCOL_CMD_ERROR);
        return;
    }

    if (frame->type != PROTOCOL_TYPE_COMMAND) {
        app_protocol_send_error(PROTOCOL_CMD_ERROR);
        return;
    }

    app_protocol_handle_command(frame);
}

void app_protocol_init(void)
{
    bsp_rtc_datetime_t default_datetime;

    app_protocol_receiver_reset();
    s_rx_error_targets_local = 0U;
    s_device_id = boot_param_get_device_id();
    s_baudrate_code = boot_param_get_baudrate_code();
    s_ch0_ratio_bits = boot_param_get_ch0_ratio_bits();
    s_ch1_ratio_bits = boot_param_get_ch1_ratio_bits();
    s_ch0_threshold_bits = boot_param_get_ch0_threshold_bits();
    s_ch1_threshold_bits = boot_param_get_ch1_threshold_bits();
    s_report_interval_code = boot_param_get_report_interval_code();
    s_report_interval_ms = app_protocol_report_interval_ms_from_code(s_report_interval_code);
    s_auto_report_enabled = 0U;
    s_auto_report_last_tick = systick_get_tick();
    s_alarm_report_mode = boot_param_get_alarm_report_mode();
    bsp_rtc_init();
    if (bsp_rtc_is_configured() == 0U) {
        default_datetime.year    = 2026U;
        default_datetime.month   = 1U;
        default_datetime.date    = 1U;
        default_datetime.weekday = 4U;
        default_datetime.hour    = 0U;
        default_datetime.minute  = 0U;
        default_datetime.second  = 0U;
        (void)bsp_rtc_set_datetime(&default_datetime);
    }
    bsp_uart1_rs485_init(boot_param_baudrate_from_code(s_baudrate_code));
}

void app_protocol_poll(void)
{
    protocol_frame_t frame;
    app_protocol_rx_status_t status;
    uint8_t data;

    while (bsp_uart1_rs485_read_byte(&data) != 0U) {
        status = app_protocol_receive_byte(data, &frame);
        if (status == APP_PROTOCOL_RX_READY) {
            app_protocol_handle_frame(&frame);
        } else if (status == APP_PROTOCOL_RX_ERROR) {
            if ((s_auto_report_enabled == 0U) && (s_rx_error_targets_local != 0U)) {
                app_protocol_send_error(PROTOCOL_CMD_ERROR);
            }
            s_rx_error_targets_local = 0U;
        }
    }

    if ((s_auto_report_enabled != 0U) &&
        ((systick_get_tick() - s_auto_report_last_tick) >= s_report_interval_ms)) {
        s_auto_report_last_tick = systick_get_tick();
        (void)app_protocol_send_report_data(0U);
    }
}

void app_protocol_send_heartbeat(void)
{
    app_protocol_send_frame(PROTOCOL_TYPE_HEARTBEAT, APP_PROTOCOL_CMD_DEVICE_HEARTBEAT, 0, 0U);
}

#endif /* CMIS_BUILD_APP */
