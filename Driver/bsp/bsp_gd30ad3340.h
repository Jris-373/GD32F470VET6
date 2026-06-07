#ifndef BSP_GD30AD3340_H
#define BSP_GD30AD3340_H

#include "gd32f4xx.h"

#define BSP_GD30AD3340_ADDR_GND 0x48U
#define BSP_GD30AD3340_ADDR_VDD 0x49U
#define BSP_GD30AD3340_ADDR_SDA 0x4AU
#define BSP_GD30AD3340_ADDR_SCL 0x4BU

typedef enum {
    BSP_GD30AD3340_CHANNEL_AIN0 = 0,
    BSP_GD30AD3340_CHANNEL_AIN1,
    BSP_GD30AD3340_CHANNEL_AIN2,
    BSP_GD30AD3340_CHANNEL_AIN3,
} bsp_gd30ad3340_channel_t;

void bsp_gd30ad3340_init(void);
uint8_t bsp_gd30ad3340_is_ready(void);
uint8_t bsp_gd30ad3340_config_differential_ain0_ain1(void);
uint8_t bsp_gd30ad3340_config_single_ended(bsp_gd30ad3340_channel_t channel);
uint8_t bsp_gd30ad3340_read_raw(int16_t *raw);
uint8_t bsp_gd30ad3340_read_voltage(float *voltage);

#endif /* BSP_GD30AD3340_H */
