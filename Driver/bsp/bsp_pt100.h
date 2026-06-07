#ifndef BSP_PT100_H
#define BSP_PT100_H

#include "gd32f4xx.h"

typedef enum {
    BSP_PT100_STATUS_NOT_READY = 0,
    BSP_PT100_STATUS_READY,
} bsp_pt100_status_t;

typedef struct {
    float voltage_to_resistance_gain;
    float voltage_to_resistance_offset;
} bsp_pt100_calibration_t;

void bsp_pt100_init(void);
bsp_pt100_status_t bsp_pt100_get_status(void);
uint8_t bsp_pt100_set_calibration(const bsp_pt100_calibration_t *calibration);
void bsp_pt100_get_calibration(bsp_pt100_calibration_t *calibration);
uint8_t bsp_pt100_read_raw(uint32_t *raw);
uint8_t bsp_pt100_read_resistance(float *resistance);
uint8_t bsp_pt100_read_temperature(float *temperature);
uint8_t bsp_pt100_read_temperature_x100(int32_t *temperature_x100);

#endif /* BSP_PT100_H */
