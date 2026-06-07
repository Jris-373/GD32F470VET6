#include "bsp_pt100.h"

void bsp_pt100_init(void)
{
}

bsp_pt100_status_t bsp_pt100_get_status(void)
{
    return BSP_PT100_STATUS_NOT_READY;
}

uint8_t bsp_pt100_read_raw(uint32_t *raw)
{
    if (raw != 0) {
        *raw = 0U;
    }

    return 0U;
}

uint8_t bsp_pt100_read_temperature_x100(int32_t *temperature_x100)
{
    if (temperature_x100 != 0) {
        *temperature_x100 = 0;
    }

    return 0U;
}
