#ifndef APP_UPDATE_H
#define APP_UPDATE_H

#include "boot_config.h"

typedef enum {
    APP_UPDATE_OK = 0,
    APP_UPDATE_TF_MOUNT_ERR,
    APP_UPDATE_TF_OPEN_ERR,
    APP_UPDATE_TF_READ_ERR,
    APP_UPDATE_FILE_EMPTY,
    APP_UPDATE_FILE_TOO_LARGE,
    APP_UPDATE_VECTOR_ERR,
    APP_UPDATE_FLASH_ERASE_ERR,
    APP_UPDATE_FLASH_WRITE_ERR,
    APP_UPDATE_VERIFY_ERR,
    APP_UPDATE_PARAM_ERR,
} app_update_status_t;

typedef enum {
    APP_UPDATE_PROGRESS_MOUNT = 0,
    APP_UPDATE_PROGRESS_OPEN,
    APP_UPDATE_PROGRESS_ERASE,
    APP_UPDATE_PROGRESS_COPY,
    APP_UPDATE_PROGRESS_VERIFY,
    APP_UPDATE_PROGRESS_PARAM,
} app_update_progress_t;

typedef void (*app_update_progress_callback_t)(app_update_progress_t progress, uint32_t done, uint32_t total);

void app_update_set_progress_callback(app_update_progress_callback_t callback);
app_update_status_t app_update_stage_tf_firmware(const char *path, uint32_t version, boot_image_header_t *out_header);

#endif /* APP_UPDATE_H */
