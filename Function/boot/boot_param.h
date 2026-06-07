#ifndef BOOT_PARAM_H
#define BOOT_PARAM_H

#include "boot_config.h"

void boot_param_default(boot_param_t *param);
uint8_t boot_param_is_valid(const boot_param_t *param);
uint8_t boot_param_load(boot_param_t *param);
uint8_t boot_param_store(const boot_param_t *param);
uint8_t boot_param_mark_pending(uint32_t slot_addr, uint32_t app_size, uint32_t app_crc32, uint32_t version);
uint8_t boot_param_mark_usart_request(void);
uint8_t boot_param_clear_update_request(void);
uint8_t boot_param_is_usart_request(void);
uint8_t boot_param_mark_app_installed(const boot_image_header_t *header);
uint8_t boot_param_confirm_app(void);
uint16_t boot_param_get_device_id(void);
uint8_t boot_param_get_baudrate_code(void);
uint32_t boot_param_baudrate_from_code(uint8_t baudrate_code);
uint8_t boot_param_set_device_id(uint16_t device_id);
uint8_t boot_param_set_baudrate_code(uint8_t baudrate_code);
uint32_t boot_param_get_ch0_ratio_bits(void);
uint32_t boot_param_get_ch1_ratio_bits(void);
uint32_t boot_param_get_ch0_threshold_bits(void);
uint32_t boot_param_get_ch1_threshold_bits(void);
uint8_t boot_param_set_ch0_ratio_bits(uint32_t ratio_bits);
uint8_t boot_param_set_ch1_ratio_bits(uint32_t ratio_bits);
uint8_t boot_param_set_ch0_threshold_bits(uint32_t threshold_bits);
uint8_t boot_param_set_ch1_threshold_bits(uint32_t threshold_bits);
uint8_t boot_param_get_report_interval_code(void);
uint8_t boot_param_set_report_interval_code(uint8_t interval_code);
uint8_t boot_param_get_alarm_report_mode(void);
uint8_t boot_param_set_alarm_report_mode(uint8_t mode);
uint16_t boot_param_get_dac_raw(void);
uint8_t boot_param_set_dac_raw(uint16_t dac_raw);
uint8_t boot_param_add_alarm(uint32_t timestamp, uint8_t channel, uint32_t threshold_bits, uint32_t actual_bits);
uint8_t boot_param_clear_alarm_records(void);

#endif /* BOOT_PARAM_H */
