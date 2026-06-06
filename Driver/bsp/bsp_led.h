#ifndef BSP_LED_H
#define BSP_LED_H

#include "gd32f4xx.h"

/*
 * The development kit exposes LED1..LED6 through H4 instead of fixing them to
 * MCU pins. Default mapping: connect H4 LED1 to PE0, or override these macros.
 */
#ifndef BSP_LED_RCU
#define BSP_LED_RCU          RCU_GPIOE
#endif

#ifndef BSP_LED_PORT
#define BSP_LED_PORT         GPIOE
#endif

#ifndef BSP_LED_PIN
#define BSP_LED_PIN          GPIO_PIN_0
#endif

#ifndef BSP_LED_ACTIVE_HIGH
#define BSP_LED_ACTIVE_HIGH  1U
#endif

void bsp_led_init(void);
void bsp_led_on(void);
void bsp_led_off(void);
void bsp_led_toggle(void);

#endif /* BSP_LED_H */
