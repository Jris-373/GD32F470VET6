#ifndef BSP_PT100_H
#define BSP_PT100_H

#include "gd32f4xx.h"

typedef enum {
    BSP_PT100_STATUS_NOT_READY = 0,
    BSP_PT100_STATUS_READY,
} bsp_pt100_status_t;

void bsp_pt100_init(void);
bsp_pt100_status_t bsp_pt100_get_status(void);
uint8_t bsp_pt100_read_raw(uint32_t *raw);
uint8_t bsp_pt100_read_temperature_x100(int32_t *temperature_x100);

#endif /* BSP_PT100_H */
