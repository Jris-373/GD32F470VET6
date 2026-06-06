#ifndef BSP_TF_H
#define BSP_TF_H

#include "bsp_rtc.h"
#include "gd32f4xx.h"

typedef enum {
    BSP_TF_OK = 0,
    BSP_TF_NOT_READY,
    BSP_TF_DISK_FAIL,
    BSP_TF_NO_FILESYSTEM,
    BSP_TF_MOUNT_FAIL,
    BSP_TF_OPEN_FAIL,
    BSP_TF_WRITE_FAIL,
    BSP_TF_READ_FAIL,
    BSP_TF_SECTOR0_FAIL,
} bsp_tf_status_t;

typedef enum {
    BSP_TF_PROGRESS_MOUNT = 0,
    BSP_TF_PROGRESS_SD_INIT,
    BSP_TF_PROGRESS_SD_POWER,
    BSP_TF_PROGRESS_SD_CMD0,
    BSP_TF_PROGRESS_SD_CMD8,
    BSP_TF_PROGRESS_SD_ACMD41,
    BSP_TF_PROGRESS_SD_CARD_INIT,
    BSP_TF_PROGRESS_SD_CID,
    BSP_TF_PROGRESS_SD_RCA,
    BSP_TF_PROGRESS_SD_CSD,
    BSP_TF_PROGRESS_SD_INFO,
    BSP_TF_PROGRESS_SD_SELECT,
    BSP_TF_PROGRESS_SD_STATUS,
    BSP_TF_PROGRESS_SD_SCR,
    BSP_TF_PROGRESS_SD_BUS,
    BSP_TF_PROGRESS_SD_MODE,
    BSP_TF_PROGRESS_SD_READ,
    BSP_TF_PROGRESS_SD_READ_CMD16,
    BSP_TF_PROGRESS_SD_READ_CMD17,
    BSP_TF_PROGRESS_SD_READ_DATA,
    BSP_TF_PROGRESS_SD_READ_FIFO,
    BSP_TF_PROGRESS_SD_WRITE,
    BSP_TF_PROGRESS_READY,
} bsp_tf_progress_t;

typedef void (*bsp_tf_progress_callback_t)(bsp_tf_progress_t progress, uint32_t value);

void bsp_tf_init(void);
void bsp_tf_set_progress_callback(bsp_tf_progress_callback_t callback);
void bsp_tf_emit_progress(bsp_tf_progress_t progress, uint32_t value);
void bsp_tf_record_progress(bsp_tf_progress_t progress);
bsp_tf_progress_t bsp_tf_get_last_progress(void);
void bsp_tf_set_last_error(uint32_t error);
uint32_t bsp_tf_get_last_error(void);
bsp_tf_status_t bsp_tf_mount(void);
bsp_tf_status_t bsp_tf_sector0_read_test(void);
bsp_tf_status_t bsp_tf_log_time(uint32_t second, const bsp_rtc_datetime_t *datetime);

#endif /* BSP_TF_H */
