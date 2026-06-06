#include "main.h"
#include "app_protocol.h"
#include "app_update.h"
#include "app_version.h"
#include "boot_image.h"
#include "boot_param.h"
#include "boot_usart_update.h"
#include "bsp.h"
#include "systick.h"

#define OLED_FONT_SIZE  16U
#define OLED_CHAR_WIDTH 8U

#ifndef BOOT_ENABLE_LEGACY_TF_UPDATE
#define BOOT_ENABLE_LEGACY_TF_UPDATE 0U
#endif

static void oled_draw_string(uint8_t x, uint8_t y, const char *str)
{
    while ((str != 0) && (*str != '\0')) {
        bsp_oled_show_char(x, y, *str, OLED_FONT_SIZE, 1U);
        x = (uint8_t)(x + OLED_CHAR_WIDTH);
        str++;
    }
}

static void oled_clear_line(uint8_t y)
{
    uint8_t index;

    for (index = 0U; index < (BSP_OLED_WIDTH / OLED_CHAR_WIDTH); index++) {
        bsp_oled_show_char((uint8_t)(index * OLED_CHAR_WIDTH), y, ' ', OLED_FONT_SIZE, 1U);
    }
}

static void oled_show_two_lines(const char *line1, const char *line2)
{
    oled_clear_line(0U);
    oled_clear_line(16U);
    oled_draw_string(0U, 0U, line1);
    if (line2 != 0) {
        oled_draw_string(0U, 16U, line2);
    }
    bsp_oled_refresh();
}

#if !defined(CMIS_BUILD_APP)
static void oled_draw_uint_fixed(uint8_t x, uint8_t y, uint32_t value, uint8_t width)
{
    char digits[10];
    uint8_t index;

    if (width > sizeof(digits)) {
        width = sizeof(digits);
    }

    for (index = 0U; index < width; index++) {
        digits[index] = '0';
    }

    index = width;
    do {
        if (index == 0U) {
            break;
        }

        index--;
        digits[index] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    for (index = 0U; index < width; index++) {
        bsp_oled_show_char((uint8_t)(x + index * OLED_CHAR_WIDTH), y, digits[index], OLED_FONT_SIZE, 1U);
    }
}

static void app_update_oled_progress(app_update_progress_t progress, uint32_t done, uint32_t total)
{
    uint32_t percent;

    switch (progress) {
    case APP_UPDATE_PROGRESS_MOUNT:
        oled_show_two_lines("MOUNT TF", "");
        break;

    case APP_UPDATE_PROGRESS_OPEN:
        oled_show_two_lines("OPEN BIN", "firmware.bin");
        break;

    case APP_UPDATE_PROGRESS_ERASE:
        oled_clear_line(0U);
        oled_clear_line(16U);
        oled_draw_string(0U, 0U, "ERASE EXT");
        oled_draw_uint_fixed(0U, 16U, done, 3U);
        oled_draw_string(24U, 16U, "/");
        oled_draw_uint_fixed(32U, 16U, total, 3U);
        bsp_oled_refresh();
        break;

    case APP_UPDATE_PROGRESS_COPY:
        percent = 0U;
        if (total != 0U) {
            percent = (done * 100U) / total;
            if (percent > 100U) {
                percent = 100U;
            }
        }
        oled_clear_line(0U);
        oled_clear_line(16U);
        oled_draw_string(0U, 0U, "COPY BIN");
        oled_draw_uint_fixed(0U, 16U, percent, 3U);
        oled_draw_string(24U, 16U, "%");
        bsp_oled_refresh();
        break;

    case APP_UPDATE_PROGRESS_VERIFY:
        oled_show_two_lines("VERIFY EXT", "");
        break;

    case APP_UPDATE_PROGRESS_PARAM:
        oled_show_two_lines("WRITE PARAM", "");
        break;

    default:
        oled_show_two_lines("UPDATING", "");
        break;
    }
}

static const char *tf_progress_text(bsp_tf_progress_t progress)
{
    switch (progress) {
    case BSP_TF_PROGRESS_MOUNT:
        return "FATFS MOUNT";

    case BSP_TF_PROGRESS_SD_INIT:
        return "SD INIT";

    case BSP_TF_PROGRESS_SD_POWER:
        return "SD POWER";

    case BSP_TF_PROGRESS_SD_CMD0:
        return "SD CMD0";

    case BSP_TF_PROGRESS_SD_CMD8:
        return "SD CMD8";

    case BSP_TF_PROGRESS_SD_ACMD41:
        return "SD ACMD41";

    case BSP_TF_PROGRESS_SD_CARD_INIT:
        return "SD CARDINIT";

    case BSP_TF_PROGRESS_SD_CID:
        return "SD CID";

    case BSP_TF_PROGRESS_SD_RCA:
        return "SD RCA";

    case BSP_TF_PROGRESS_SD_CSD:
        return "SD CSD";

    case BSP_TF_PROGRESS_SD_INFO:
        return "SD INFO";

    case BSP_TF_PROGRESS_SD_SELECT:
        return "SD SELECT";

    case BSP_TF_PROGRESS_SD_STATUS:
        return "SD STATUS";

    case BSP_TF_PROGRESS_SD_SCR:
        return "SD SCR";

    case BSP_TF_PROGRESS_SD_BUS:
        return "SD BUS";

    case BSP_TF_PROGRESS_SD_MODE:
        return "SD MODE";

    case BSP_TF_PROGRESS_SD_READ:
        return "SD READ";

    case BSP_TF_PROGRESS_SD_READ_CMD16:
        return "READ CMD16";

    case BSP_TF_PROGRESS_SD_READ_CMD17:
        return "READ CMD17";

    case BSP_TF_PROGRESS_SD_READ_DATA:
        return "READ DATA";

    case BSP_TF_PROGRESS_SD_READ_FIFO:
        return "READ FIFO";

    case BSP_TF_PROGRESS_SD_WRITE:
        return "SD WRITE";

    case BSP_TF_PROGRESS_READY:
        return "TF READY";

    default:
        return "TF UNKNOWN";
    }
}

static const char *tf_progress_short_text(bsp_tf_progress_t progress)
{
    switch (progress) {
    case BSP_TF_PROGRESS_SD_INIT:
        return "INIT";

    case BSP_TF_PROGRESS_SD_POWER:
        return "POWER";

    case BSP_TF_PROGRESS_SD_CMD0:
        return "CMD0";

    case BSP_TF_PROGRESS_SD_CMD8:
        return "CMD8";

    case BSP_TF_PROGRESS_SD_ACMD41:
        return "ACMD41";

    case BSP_TF_PROGRESS_SD_CARD_INIT:
        return "CARD";

    case BSP_TF_PROGRESS_SD_CID:
        return "CID";

    case BSP_TF_PROGRESS_SD_RCA:
        return "RCA";

    case BSP_TF_PROGRESS_SD_CSD:
        return "CSD";

    case BSP_TF_PROGRESS_SD_INFO:
        return "INFO";

    case BSP_TF_PROGRESS_SD_SELECT:
        return "SEL";

    case BSP_TF_PROGRESS_SD_STATUS:
        return "STAT";

    case BSP_TF_PROGRESS_SD_SCR:
        return "SCR";

    case BSP_TF_PROGRESS_SD_BUS:
        return "BUS";

    case BSP_TF_PROGRESS_SD_MODE:
        return "MODE";

    case BSP_TF_PROGRESS_SD_READ:
        return "READ";

    case BSP_TF_PROGRESS_SD_READ_CMD16:
        return "RC16";

    case BSP_TF_PROGRESS_SD_READ_CMD17:
        return "RC17";

    case BSP_TF_PROGRESS_SD_READ_DATA:
        return "RDATA";

    case BSP_TF_PROGRESS_SD_READ_FIFO:
        return "RFIFO";

    case BSP_TF_PROGRESS_SD_WRITE:
        return "WRITE";

    default:
        return "TF";
    }
}

static void tf_oled_progress(bsp_tf_progress_t progress, uint32_t value)
{
    (void)value;
    oled_show_two_lines("MOUNT TF", tf_progress_text(progress));
}

static void tf_dma_oled_progress(bsp_tf_progress_t progress, uint32_t value)
{
    (void)value;
    oled_show_two_lines("DMA S0 TEST", tf_progress_text(progress));
}

static void oled_show_tf_error(const char *title)
{
    uint32_t error;

    error = bsp_tf_get_last_error();
    oled_clear_line(0U);
    oled_clear_line(16U);
    oled_draw_string(0U, 0U, title);
    oled_draw_string(0U, 16U, tf_progress_short_text(bsp_tf_get_last_progress()));
    oled_draw_string(48U, 16U, "E:");
    oled_draw_uint_fixed(64U, 16U, error, 2U);
    bsp_oled_refresh();
}

static void oled_show_tf_mount_error(void)
{
    oled_show_tf_error("TF MOUNT ERR");
}

static uint8_t tf_dma_sector0_preflight(void)
{
    bsp_tf_status_t status;

    bsp_tf_set_progress_callback(tf_dma_oled_progress);
    status = bsp_tf_sector0_read_test();
    bsp_tf_set_progress_callback(0);

    if (status == BSP_TF_OK) {
        oled_show_two_lines("DMA S0 OK", "SECTOR0 OK");
        delay_1ms(300U);
        return 1U;
    }

    oled_show_tf_error("DMA S0 FAIL");
    delay_1ms(1000U);
    return 0U;
}

static void key_wait_release_id(bsp_key_id_t key_id)
{
    while (bsp_key_is_pressed_id(key_id) != 0U) {
    }

    delay_1ms(BSP_KEY_DEBOUNCE_MS + 5U);
    while (bsp_key_scan_event_id(key_id) != BSP_KEY_EVENT_NONE) {
    }
}

static const char *update_status_text(app_update_status_t status)
{
    switch (status) {
    case APP_UPDATE_OK:
        return "STAGE OK";

    case APP_UPDATE_TF_MOUNT_ERR:
        return "TF MOUNT ERR";

    case APP_UPDATE_TF_OPEN_ERR:
        return "TF OPEN ERR";

    case APP_UPDATE_TF_READ_ERR:
        return "TF READ ERR";

    case APP_UPDATE_FILE_EMPTY:
        return "BIN EMPTY";

    case APP_UPDATE_FILE_TOO_LARGE:
        return "BIN TOO LARGE";

    case APP_UPDATE_VECTOR_ERR:
        return "BIN VEC ERR";

    case APP_UPDATE_FLASH_ERASE_ERR:
        return "EXT ERASE ERR";

    case APP_UPDATE_FLASH_WRITE_ERR:
        return "EXT WRITE ERR";

    case APP_UPDATE_VERIFY_ERR:
        return "VERIFY ERR";

    case APP_UPDATE_PARAM_ERR:
        return "PARAM ERR";

    default:
        return "UPDATE ERR";
    }
}
#endif

#if defined(CMIS_BUILD_APP)
static void app_show_status(const char *status)
{
    oled_show_two_lines(APP_TEAM_ID_TEXT, status);
}

int main(void)
{
    uint32_t last_tick;
    systick_config();
    bsp_led_init();
    bsp_key_init();
    bsp_adc_init();
    bsp_dac_init();
    bsp_pt100_init();
    bsp_oled_init();
    bsp_spi_flash_init();
    bsp_tf_init();
    app_protocol_init();

    (void)boot_param_confirm_app();
    app_show_status(APP_STATUS_IDLE);
    app_protocol_send_heartbeat();

    last_tick = systick_get_tick();

    while (1) {
        app_protocol_poll();

        if ((systick_get_tick() - last_tick) >= 1000U) {
            last_tick += 1000U;
            bsp_led_toggle();
        }
    }
}

#else

static void boot_try_pending_update(void)
{
    boot_param_t param;
    boot_status_t status;
    boot_image_header_t header;

    (void)boot_param_load(&param);
    if (param.update_flag != BOOT_UPDATE_FLAG_PENDING) {
        return;
    }

    oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
    status = boot_apply_external_image(param.update_slot_addr, &header);
    if (status == BOOT_OK) {
        oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
        delay_1ms(500U);
        NVIC_SystemReset();
    }

    (void)status;
    oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
    delay_1ms(1000U);
}

static void boot_show_wait_update(void)
{
    oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
}

static void boot_show_no_app(void)
{
    oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
}

static void boot_show_usart_fail(void)
{
    oled_show_two_lines(APP_TEAM_ID_TEXT, APP_STATUS_BOOTLOADER);
}

static void boot_try_jump_app(void)
{
    if (boot_app_is_present() == 0U) {
        boot_show_no_app();
        delay_1ms(1000U);
        return;
    }

    bsp_oled_clear();
    bsp_oled_refresh();
    delay_1ms(50U);
    boot_jump_to_app(BOOT_APP_START_ADDR);
}

static void boot_stage_firmware_from_tf(void)
{
    app_update_status_t status;
    boot_image_header_t header;

    key_wait_release_id(BSP_KEY_ID_1);
    oled_show_two_lines("TF READING", "firmware.bin");

    if (tf_dma_sector0_preflight() == 0U) {
        oled_show_two_lines("TF FAIL", "SD PREFLIGHT");
        return;
    }

    app_update_set_progress_callback(app_update_oled_progress);
    bsp_tf_set_progress_callback(tf_oled_progress);
    status = app_update_stage_tf_firmware(BOOT_TF_FIRMWARE_PATH, 1U, &header);
    bsp_tf_set_progress_callback(0);
    app_update_set_progress_callback(0);
    if (status == APP_UPDATE_OK) {
        oled_show_two_lines("STAGE OK", "RESET");
        delay_1ms(500U);
        NVIC_SystemReset();
    }

    if (status == APP_UPDATE_TF_MOUNT_ERR) {
        oled_show_tf_mount_error();
    } else {
        oled_show_two_lines("STAGE FAIL", update_status_text(status));
    }
    delay_1ms(1000U);
    boot_show_wait_update();
}

static void boot_poll_keys(void)
{
    volatile uint8_t legacy_tf_update_enabled = BOOT_ENABLE_LEGACY_TF_UPDATE;

    if (legacy_tf_update_enabled == 0U) {
        return;
    }

    if (bsp_key_scan_event_id(BSP_KEY_ID_1) == BSP_KEY_EVENT_PRESSED) {
        boot_stage_firmware_from_tf();
    }

    if (bsp_key_scan_event_id(BSP_KEY_ID_2) == BSP_KEY_EVENT_PRESSED) {
        key_wait_release_id(BSP_KEY_ID_2);
        boot_try_jump_app();
        boot_show_wait_update();
    }
}

int main(void)
{
    uint32_t last_tick;
    uint32_t menu_start_tick;
    uint8_t stay_bootloader_after_usart;

    systick_config();
    bsp_led_init();
    bsp_oled_init();
    bsp_spi_flash_init();
    boot_usart_protocol_init();

    bsp_oled_clear();
    bsp_oled_refresh();
    boot_show_wait_update();

    boot_try_pending_update();

    stay_bootloader_after_usart = 0U;
    if (boot_usart_update_requested() != 0U) {
        boot_show_wait_update();
        if (boot_usart_bootloader_upgrade_window() == 0U) {
            boot_try_jump_app();
        } else {
            boot_show_usart_fail();
            stay_bootloader_after_usart = 1U;
        }
    }

    last_tick = systick_get_tick();
    menu_start_tick = last_tick;

    while ((stay_bootloader_after_usart == 0U) &&
           (boot_app_is_present() != 0U) &&
           ((systick_get_tick() - menu_start_tick) < 1000U)) {
        boot_poll_keys();

        if ((systick_get_tick() - last_tick) >= 500U) {
            last_tick += 500U;
            bsp_led_toggle();
        }
    }

    if ((stay_bootloader_after_usart == 0U) && (boot_app_is_present() != 0U)) {
        boot_try_jump_app();
    }

    if (boot_app_is_present() == 0U) {
        boot_show_no_app();
    } else if (stay_bootloader_after_usart != 0U) {
        boot_show_usart_fail();
    } else {
        boot_show_wait_update();
    }

    last_tick = systick_get_tick();

    while (1) {
        boot_poll_keys();

        if ((systick_get_tick() - last_tick) >= 500U) {
            last_tick += 500U;
            bsp_led_toggle();
        }
    }
}

#endif
