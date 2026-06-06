#ifndef BSP_POWER_H
#define BSP_POWER_H

#include "gd32f4xx.h"

void bsp_power_sleep(void);
uint8_t bsp_power_deepsleep_rtc_alarm(uint8_t seconds);

#endif /* BSP_POWER_H */
