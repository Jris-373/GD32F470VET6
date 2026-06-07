#ifndef BSP_LED_H
#define BSP_LED_H

#include "gd32f4xx.h"

/*
 * The development kit exposes LED1..LED6 through H4 instead of fixing them to
 * MCU pins. Default mapping: connect H4 LED1 to PE0, or override these macros.
 * Sample-status LED default mapping: connect H4 LED2 to PE4. If PE4 is already
 * used by another jumper on a specific board setup, override BSP_LED_SAMPLE_*
 * macros to move the sample LED to another free H4 GPIO.
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

#ifndef BSP_LED_SAMPLE_RCU
#define BSP_LED_SAMPLE_RCU          RCU_GPIOE
#endif

#ifndef BSP_LED_SAMPLE_PORT
#define BSP_LED_SAMPLE_PORT         GPIOE
#endif

#ifndef BSP_LED_SAMPLE_PIN
#define BSP_LED_SAMPLE_PIN          GPIO_PIN_4
#endif

#ifndef BSP_LED_SAMPLE_ACTIVE_HIGH
#define BSP_LED_SAMPLE_ACTIVE_HIGH  1U
#endif

void bsp_led_init(void);
void bsp_led_on(void);
void bsp_led_off(void);
void bsp_led_toggle(void);
void bsp_led_sample_on(void);
void bsp_led_sample_off(void);
void bsp_led_sample_toggle(void);

#endif /* BSP_LED_H */
