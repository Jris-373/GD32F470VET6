#ifndef BSP_DAC_H
#define BSP_DAC_H

#include "gd32f4xx.h"

#ifndef BSP_DAC_RCU
#define BSP_DAC_RCU           RCU_DAC
#endif

#ifndef BSP_DAC_PERIPH
#define BSP_DAC_PERIPH        DAC0
#endif

#ifndef BSP_DAC_OUT
#define BSP_DAC_OUT           DAC_OUT0
#endif

#ifndef BSP_DAC_GPIO_RCU
#define BSP_DAC_GPIO_RCU      RCU_GPIOA
#endif

#ifndef BSP_DAC_GPIO_PORT
#define BSP_DAC_GPIO_PORT     GPIOA
#endif

#ifndef BSP_DAC_GPIO_PIN
#define BSP_DAC_GPIO_PIN      GPIO_PIN_4
#endif

#ifndef BSP_DAC_VREF_MV
#define BSP_DAC_VREF_MV       3300U
#endif

#define BSP_DAC_MAX_RAW       4095U

void bsp_dac_init(void);
void bsp_dac_write_raw(uint16_t raw);
void bsp_dac_write_mv(uint16_t millivolt);
void bsp_dac_write_percent(uint8_t percent);
uint16_t bsp_dac_get_raw(void);
uint16_t bsp_dac_get_mv(void);
uint16_t bsp_dac_mv_to_raw(uint16_t millivolt);
uint16_t bsp_dac_raw_to_mv(uint16_t raw);

#endif /* BSP_DAC_H */
