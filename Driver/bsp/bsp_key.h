#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "gd32f4xx.h"

/*
 * The development kit exposes FUN_KEY1..FUN_KEY6 through H3 instead of fixing
 * them to MCU pins. Default mapping:
 * - connect H3 FUN_KEY1 to PE1
 * - connect H3 FUN_KEY2 to PE3
 */
#ifndef BSP_KEY1_RCU
#define BSP_KEY1_RCU        RCU_GPIOE
#endif

#ifndef BSP_KEY1_PORT
#define BSP_KEY1_PORT       GPIOE
#endif

#ifndef BSP_KEY1_PIN
#define BSP_KEY1_PIN        GPIO_PIN_1
#endif

#ifndef BSP_KEY2_RCU
#define BSP_KEY2_RCU        RCU_GPIOE
#endif

#ifndef BSP_KEY2_PORT
#define BSP_KEY2_PORT       GPIOE
#endif

#ifndef BSP_KEY2_PIN
#define BSP_KEY2_PIN        GPIO_PIN_3
#endif

#ifndef BSP_KEY_ACTIVE_LOW
#define BSP_KEY_ACTIVE_LOW  1U
#endif

#ifndef BSP_KEY_DEBOUNCE_MS
#define BSP_KEY_DEBOUNCE_MS 20U
#endif

#ifndef BSP_KEY_SYSCFG_RCU
#define BSP_KEY_SYSCFG_RCU RCU_SYSCFG
#endif

#ifndef BSP_KEY_EXTI_PORT_SOURCE
#define BSP_KEY_EXTI_PORT_SOURCE EXTI_SOURCE_GPIOE
#endif

#ifndef BSP_KEY_EXTI_PIN_SOURCE
#define BSP_KEY_EXTI_PIN_SOURCE EXTI_SOURCE_PIN1
#endif

#ifndef BSP_KEY_EXTI_LINE
#define BSP_KEY_EXTI_LINE EXTI_1
#endif

#ifndef BSP_KEY_EXTI_IRQn
#define BSP_KEY_EXTI_IRQn EXTI1_IRQn
#endif

#define BSP_KEY_RCU         BSP_KEY1_RCU
#define BSP_KEY_PORT        BSP_KEY1_PORT
#define BSP_KEY_PIN         BSP_KEY1_PIN

typedef enum {
    BSP_KEY_ID_1 = 0,
    BSP_KEY_ID_2,
    BSP_KEY_ID_COUNT
} bsp_key_id_t;

typedef enum {
    BSP_KEY_EVENT_NONE = 0,
    BSP_KEY_EVENT_PRESSED,
    BSP_KEY_EVENT_RELEASED,
} bsp_key_event_t;

void bsp_key_init(void);
uint8_t bsp_key_read_id(bsp_key_id_t key_id);
uint8_t bsp_key_is_pressed_id(bsp_key_id_t key_id);
bsp_key_event_t bsp_key_scan_event_id(bsp_key_id_t key_id);
uint8_t bsp_key_was_pressed_id(bsp_key_id_t key_id);
uint8_t bsp_key_read(void);
uint8_t bsp_key_is_pressed(void);
bsp_key_event_t bsp_key_scan_event(void);
uint8_t bsp_key_was_pressed(void);
void bsp_key_exti_irq_handler(void);

#endif /* BSP_KEY_H */
