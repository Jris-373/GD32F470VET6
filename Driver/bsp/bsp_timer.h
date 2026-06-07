#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include "gd32f4xx.h"

#define BSP_TIMER_PERIPH     TIMER6
#define BSP_TIMER_RCU        RCU_TIMER6
#define BSP_TIMER_IRQn       TIMER6_IRQn
#define BSP_TIMER_BASE_HZ    10000U

void bsp_timer_init(uint32_t period_ms);
void bsp_timer_start(void);
void bsp_timer_stop(void);
void bsp_timer_clear_elapsed(void);
uint8_t bsp_timer_elapsed(void);
uint32_t bsp_timer_get_tick(void);
void bsp_timer_irq_handler(void);

#endif /* BSP_TIMER_H */
