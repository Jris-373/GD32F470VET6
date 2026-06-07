#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

#include "gd32f4xx.h"

#define BOOTLOADER_START_ADDR         0x08000000U
#define BOOTLOADER_SIZE               0x00010000U
#define BOOT_PARAM_ADDR               0x08010000U
#define BOOT_PARAM_SIZE               0x00001000U
#define BOOT_APP_START_ADDR           0x08011000U
#define BOOT_APP_SIZE                 0x00020000U
#define BOOT_APP_BACKUP_ADDR          0x08031000U
#define BOOT_APP_BACKUP_SIZE          0x00020000U
#define BOOT_FW_STAGING_ADDR          0x08051000U
#define BOOT_FW_STAGING_SIZE          0x00020000U
#define BOOT_INTERNAL_FLASH_END_ADDR  0x08080000U
#define BOOT_INTERNAL_FLASH_PAGE_SIZE 0x00001000U

#define BOOT_SRAM_START_ADDR          0x20000000U
#define BOOT_SRAM_END_ADDR            0x20030000U

#define BOOT_EXT_FLASH_SIZE           0x00080000U
#define BOOT_EXT_SLOT0_ADDR           0x00000000U
#define BOOT_EXT_SLOT0_SIZE           BOOT_EXT_FLASH_SIZE
#define BOOT_IMAGE_HEADER_BYTES       0x00000100U
#define BOOT_EXT_SLOT0_DATA_ADDR      (BOOT_EXT_SLOT0_ADDR + BOOT_IMAGE_HEADER_BYTES)

#define BOOT_APP_END_ADDR             (BOOT_APP_START_ADDR + BOOT_APP_SIZE)
#define BOOT_APP_BACKUP_END_ADDR      (BOOT_APP_BACKUP_ADDR + BOOT_APP_BACKUP_SIZE)
#define BOOT_FW_STAGING_END_ADDR      (BOOT_FW_STAGING_ADDR + BOOT_FW_STAGING_SIZE)
#define BOOT_APP_MAX_SIZE             BOOT_APP_SIZE
#define BOOT_TF_FIRMWARE_PATH         "0:/firmware.bin"

#define BOOT_PARAM_MAGIC              0x5AA5C33CU
#define BOOT_PARAM_VERSION            5U
#define BOOT_PARAM_LEGACY_VERSION     2U
#define BOOT_PARAM_LEGACY_V3_VERSION  3U
#define BOOT_PARAM_LEGACY_V4_VERSION  4U
#define BOOT_PARAM_TAIL_MAGIC         0xA5A5C3C3U
#define BOOT_FW_PACKAGE_MAGIC         0x5AA5C33CU
#define BOOT_IMAGE_MAGIC              0x53494D43U
#define BOOT_IMAGE_TAIL_MAGIC         0xC3C3A5A5U

#define BOOT_UPDATE_FLAG_NONE         0xFFFFFFFFU
#define BOOT_UPDATE_FLAG_PENDING      0xA55A5AA5U
#define BOOT_UPDATE_FLAG_USART_REQUEST 0xA55A0501U
#define BOOT_IMAGE_STATE_READY        0x3CC35AA5U
#define BOOT_APP_CONFIRM_NONE         0xFFFFFFFFU
#define BOOT_APP_CONFIRM_OK           0xC0DEF00DU

#define BOOT_DEVICE_DEFAULT_ID        0x0001U
#define BOOT_USART_DEFAULT_BAUDRATE   19200U
#define BOOT_USART_DEFAULT_BAUD_CODE  0x13U
#define BOOT_USART_BAUD_CODE_4800     0x11U
#define BOOT_USART_BAUD_CODE_9600     0x12U
#define BOOT_USART_BAUD_CODE_19200    0x13U
#define BOOT_USART_BAUD_CODE_115200   0x14U
#define BOOT_APP_START_DELAY_MS       5000U
#define BOOT_USART_BOOT_WINDOW_MS     10000U
#define BOOT_USART_FW_FIRST_TIMEOUT_MS 30000U
#define BOOT_USART_FW_READY_ACK_TIMEOUT_MS 700U
#define BOOT_USART_FW_IDLE_TIMEOUT_MS 500U
#define BOOT_USART_WAIT_0503_MS       120000U
#define BOOT_PARAM_FLOAT_1_BITS       0x3F800000U
#define BOOT_PARAM_FLOAT_4095_BITS    0x457FF000U
#define BOOT_PARAM_FLOAT_850_BITS     0x44548000U
#define BOOT_PARAM_PT100_GAIN_BITS    0x447B9284U
#define BOOT_PARAM_PT100_OFFSET_BITS  0x00000000U
#define BOOT_REPORT_INTERVAL_1S       0x01U
#define BOOT_REPORT_INTERVAL_3S       0x02U
#define BOOT_REPORT_INTERVAL_5S       0x03U
#define BOOT_REPORT_INTERVAL_DEFAULT  BOOT_REPORT_INTERVAL_1S
#define BOOT_ALARM_MODE_ACTIVE        0x01U
#define BOOT_ALARM_MODE_PASSIVE       0x02U
#define BOOT_ALARM_MODE_DEFAULT       BOOT_ALARM_MODE_PASSIVE
#define BOOT_ALARM_RECORD_MAX         10U
#define BOOT_DAC_RAW_DEFAULT           0U
#define BOOT_DAC_RAW_MAX               4095U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t update_flag;
    uint32_t update_slot_addr;
    uint32_t app_start_addr;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
    uint32_t confirm_magic;
    uint32_t boot_count;
    uint32_t boot_fail_count;
    uint32_t last_error;
    uint16_t device_id;
    uint8_t baudrate_code;
    uint8_t reserved0;
    uint32_t ch0_ratio_bits;
    uint32_t ch1_ratio_bits;
    uint32_t ch0_threshold_bits;
    uint32_t ch1_threshold_bits;
    uint8_t report_interval_code;
    uint8_t alarm_report_mode;
    uint8_t alarm_count;
    uint8_t alarm_reserved0;
    uint32_t alarm_timestamp[BOOT_ALARM_RECORD_MAX];
    uint8_t alarm_channel[BOOT_ALARM_RECORD_MAX];
    uint16_t dac_raw;
    uint32_t alarm_threshold_bits[BOOT_ALARM_RECORD_MAX];
    uint32_t alarm_actual_bits[BOOT_ALARM_RECORD_MAX];
    uint32_t ch2_threshold_bits;
    uint32_t pt100_v_to_r_gain_bits;
    uint32_t pt100_v_to_r_offset_bits;
    uint32_t tail_magic;
} boot_param_t;

typedef struct {
    uint32_t magic;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t image_version;
    uint32_t target_addr;
    uint32_t stack_addr;
    uint32_t entry_addr;
    uint32_t state;
    uint32_t sequence;
    uint32_t tail_magic;
    uint32_t reserved[52];
} boot_image_header_t;

typedef enum {
    BOOT_OK = 0,
    BOOT_ERR_PARAM,
    BOOT_ERR_IMAGE_HEADER,
    BOOT_ERR_IMAGE_VECTOR,
    BOOT_ERR_IMAGE_CRC,
    BOOT_ERR_IMAGE_SIZE,
    BOOT_ERR_FLASH_ERASE,
    BOOT_ERR_FLASH_WRITE,
    BOOT_ERR_FLASH_READ,
    BOOT_ERR_NO_APP,
} boot_status_t;

#endif /* BOOT_CONFIG_H */
