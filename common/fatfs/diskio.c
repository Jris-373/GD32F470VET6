#include "diskio.h"
#include "bsp_tf.h"
#include "sdcard.h"

#define TF_DRIVE          0U
#define TF_BLOCK_SIZE     512U
#define TF_ERASE_BLK_SIZE 1U
#define TF_INIT_RETRY     5U

static DSTATUS s_tf_status = STA_NOINIT;
static sd_card_info_struct s_tf_card_info;
static uint32_t s_tf_sector_words[TF_BLOCK_SIZE / sizeof(uint32_t)];

static DRESULT tf_result_from_sd(sd_error_enum error)
{
    return (error == SD_OK) ? RES_OK : RES_ERROR;
}

static void tf_copy_bytes(BYTE *dest, const BYTE *src, UINT length)
{
    UINT index;

    for (index = 0U; index < length; index++) {
        dest[index] = src[index];
    }
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != TF_DRIVE) {
        return STA_NOINIT;
    }

    return s_tf_status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    sd_error_enum error;
    uint8_t retry;
    uint32_t cardstate;

    if (pdrv != TF_DRIVE) {
        return STA_NOINIT;
    }

    error = SD_ERROR;
    for (retry = 0U; retry < TF_INIT_RETRY; retry++) {
        cardstate = 0U;

        bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_INIT, retry + 1U);
        error = sd_init();
        if (error == SD_OK) {
            bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_INFO, retry + 1U);
            error = sd_card_information_get(&s_tf_card_info);
        }
        if (error != SD_OK) {
            bsp_tf_set_last_error((uint32_t)error);
            continue;
        }
        if (error == SD_OK) {
            bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_STATUS, retry + 1U);
            error = sd_cardstatus_get(&cardstate);
        }
        if ((error == SD_OK) && ((cardstate & 0x02000000U) != 0U)) {
            error = SD_LOCK_UNLOCK_FAILED;
        }
        if (error != SD_OK) {
            bsp_tf_set_last_error((uint32_t)error);
            continue;
        }
        if (error == SD_OK) {
            /* 1-bit mode is enough for logging and avoids depending on DAT1..DAT3 during bring-up. */
            bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_BUS, retry + 1U);
            error = sd_bus_mode_config(SDIO_BUSMODE_1BIT);
        }
        if (error != SD_OK) {
            bsp_tf_set_last_error((uint32_t)error);
            continue;
        }
        if (error == SD_OK) {
            bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_MODE, retry + 1U);
            error = sd_transfer_mode_config(SD_DMA_MODE);
        }
        if (error != SD_OK) {
            bsp_tf_set_last_error((uint32_t)error);
            continue;
        }
        if (error == SD_OK) {
            break;
        }
    }

    if (error != SD_OK) {
        s_tf_status = STA_NOINIT;
        return s_tf_status;
    }
    s_tf_status = 0U;
    bsp_tf_set_last_error(0U);
    bsp_tf_emit_progress(BSP_TF_PROGRESS_READY, 0U);

    return s_tf_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    UINT index;
    DRESULT result;
    sd_error_enum error;

    if ((pdrv != TF_DRIVE) || (buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }

    if ((s_tf_status & STA_NOINIT) != 0U) {
        return RES_NOTRDY;
    }

    for (index = 0U; index < count; index++) {
        bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_READ, sector + index);
        error = sd_block_read(s_tf_sector_words, sector + index, TF_BLOCK_SIZE);
        bsp_tf_set_last_error((uint32_t)error);
        result = tf_result_from_sd(error);
        if (result != RES_OK) {
            return result;
        }
        tf_copy_bytes(&buff[index * TF_BLOCK_SIZE], (const BYTE *)s_tf_sector_words, TF_BLOCK_SIZE);
    }

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    UINT index;
    DRESULT result;
    sd_error_enum error;

    if ((pdrv != TF_DRIVE) || (buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }

    if ((s_tf_status & STA_NOINIT) != 0U) {
        return RES_NOTRDY;
    }

    for (index = 0U; index < count; index++) {
        tf_copy_bytes((BYTE *)s_tf_sector_words, &buff[index * TF_BLOCK_SIZE], TF_BLOCK_SIZE);
        bsp_tf_emit_progress(BSP_TF_PROGRESS_SD_WRITE, sector + index);
        error = sd_block_write(s_tf_sector_words, sector + index, TF_BLOCK_SIZE);
        bsp_tf_set_last_error((uint32_t)error);
        result = tf_result_from_sd(error);
        if (result != RES_OK) {
            return result;
        }
    }

    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != TF_DRIVE) {
        return RES_PARERR;
    }

    if ((s_tf_status & STA_NOINIT) != 0U) {
        return RES_NOTRDY;
    }

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_COUNT:
        if (buff == 0) {
            return RES_PARERR;
        }
        *(DWORD *)buff = (DWORD)(sd_card_capacity_get() * 2U);
        return RES_OK;

    case GET_SECTOR_SIZE:
        if (buff == 0) {
            return RES_PARERR;
        }
        *(WORD *)buff = TF_BLOCK_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:
        if (buff == 0) {
            return RES_PARERR;
        }
        *(DWORD *)buff = TF_ERASE_BLK_SIZE;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}
