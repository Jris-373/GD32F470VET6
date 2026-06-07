#include "bsp_key.h"
#include "systick.h"

typedef struct {
    uint32_t rcu;
    uint32_t port;
    uint32_t pin;
} bsp_key_hw_t;

static const bsp_key_hw_t s_key_hw[BSP_KEY_ID_COUNT] = {
    {BSP_KEY1_RCU, BSP_KEY1_PORT, BSP_KEY1_PIN},
    {BSP_KEY2_RCU, BSP_KEY2_PORT, BSP_KEY2_PIN},
};

static uint8_t s_key_stable[BSP_KEY_ID_COUNT];
static uint8_t s_key_sample[BSP_KEY_ID_COUNT];
static uint32_t s_key_sample_tick[BSP_KEY_ID_COUNT];

static uint8_t bsp_key_id_is_valid(bsp_key_id_t key_id)
{
    return ((uint32_t)key_id < (uint32_t)BSP_KEY_ID_COUNT) ? 1U : 0U;
}

void bsp_key_init(void)
{
    uint8_t index;
    uint32_t current_tick;

    rcu_periph_clock_enable(BSP_KEY1_RCU);
    rcu_periph_clock_enable(BSP_KEY2_RCU);
    rcu_periph_clock_enable(BSP_KEY_SYSCFG_RCU);

    for (index = 0U; index < (uint8_t)BSP_KEY_ID_COUNT; index++) {
        gpio_mode_set(s_key_hw[index].port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, s_key_hw[index].pin);
    }

    syscfg_exti_line_config(BSP_KEY_EXTI_PORT_SOURCE, BSP_KEY_EXTI_PIN_SOURCE);
    exti_interrupt_disable(BSP_KEY_EXTI_LINE);
    exti_interrupt_flag_clear(BSP_KEY_EXTI_LINE);
    exti_init(BSP_KEY_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    nvic_irq_enable(BSP_KEY_EXTI_IRQn, 2U, 0U);

    current_tick = systick_get_tick();
    for (index = 0U; index < (uint8_t)BSP_KEY_ID_COUNT; index++) {
        s_key_stable[index] = bsp_key_is_pressed_id((bsp_key_id_t)index);
        s_key_sample[index] = s_key_stable[index];
        s_key_sample_tick[index] = current_tick;
    }
}

uint8_t bsp_key_read_id(bsp_key_id_t key_id)
{
    if (bsp_key_id_is_valid(key_id) == 0U) {
        return 1U;
    }

    return (gpio_input_bit_get(s_key_hw[key_id].port, s_key_hw[key_id].pin) == SET) ? 1U : 0U;
}

uint8_t bsp_key_is_pressed_id(bsp_key_id_t key_id)
{
#if BSP_KEY_ACTIVE_LOW
    return (bsp_key_read_id(key_id) == 0U) ? 1U : 0U;
#else
    return (bsp_key_read_id(key_id) != 0U) ? 1U : 0U;
#endif
}

bsp_key_event_t bsp_key_scan_event_id(bsp_key_id_t key_id)
{
    uint8_t pressed;
    uint32_t current_tick;

    if (bsp_key_id_is_valid(key_id) == 0U) {
        return BSP_KEY_EVENT_NONE;
    }

    current_tick = systick_get_tick();
    pressed = bsp_key_is_pressed_id(key_id);

    if (pressed != s_key_sample[key_id]) {
        s_key_sample[key_id] = pressed;
        s_key_sample_tick[key_id] = current_tick;
        return BSP_KEY_EVENT_NONE;
    }

    if ((pressed != s_key_stable[key_id]) && ((current_tick - s_key_sample_tick[key_id]) >= BSP_KEY_DEBOUNCE_MS)) {
        s_key_stable[key_id] = pressed;
        return (pressed != 0U) ? BSP_KEY_EVENT_PRESSED : BSP_KEY_EVENT_RELEASED;
    }

    return BSP_KEY_EVENT_NONE;
}

uint8_t bsp_key_was_pressed_id(bsp_key_id_t key_id)
{
    return (bsp_key_scan_event_id(key_id) == BSP_KEY_EVENT_PRESSED) ? 1U : 0U;
}

uint8_t bsp_key_read(void)
{
    return bsp_key_read_id(BSP_KEY_ID_1);
}

uint8_t bsp_key_is_pressed(void)
{
    return bsp_key_is_pressed_id(BSP_KEY_ID_1);
}

bsp_key_event_t bsp_key_scan_event(void)
{
    return bsp_key_scan_event_id(BSP_KEY_ID_1);
}

uint8_t bsp_key_was_pressed(void)
{
    return bsp_key_was_pressed_id(BSP_KEY_ID_1);
}

void bsp_key_exti_irq_handler(void)
{
    if (exti_interrupt_flag_get(BSP_KEY_EXTI_LINE) == RESET) {
        return;
    }

    exti_interrupt_flag_clear(BSP_KEY_EXTI_LINE);
}
