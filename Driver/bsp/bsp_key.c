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

/*
 * 检查按键编号是否在 BSP 支持范围内。
 * 参数 key_id 为按键枚举值；返回 1 表示有效，返回 0 表示越界。
 */
static uint8_t bsp_key_id_is_valid(bsp_key_id_t key_id)
{
    return ((uint32_t)key_id < (uint32_t)BSP_KEY_ID_COUNT) ? 1U : 0U;
}

/*
 * 初始化按键 GPIO、外部中断和消抖状态。
 * 无传入参数，无返回值；默认按键使用上拉输入，EXTI 用于唤醒或快速响应 KEY1。
 */
void bsp_key_init(void)
{
    uint8_t index;
    uint32_t current_tick;

    rcu_periph_clock_enable(BSP_KEY1_RCU);
    rcu_periph_clock_enable(BSP_KEY2_RCU);
    rcu_periph_clock_enable(BSP_KEY_SYSCFG_RCU);

    /* 两个功能键共用同一套消抖逻辑，初始化时先读取一次当前稳定状态。 */
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

/*
 * 读取指定按键 GPIO 的原始电平。
 * 参数 key_id 指定按键编号；返回 1 表示引脚为高电平，返回 0 表示低电平，编号无效时按未按下处理。
 */
uint8_t bsp_key_read_id(bsp_key_id_t key_id)
{
    if (bsp_key_id_is_valid(key_id) == 0U) {
        return 1U;
    }

    return (gpio_input_bit_get(s_key_hw[key_id].port, s_key_hw[key_id].pin) == SET) ? 1U : 0U;
}

/*
 * 判断指定按键是否处于按下状态。
 * 参数 key_id 指定按键编号；返回 1 表示按下，返回 0 表示未按下，会根据 BSP_KEY_ACTIVE_LOW 处理有效电平。
 */
uint8_t bsp_key_is_pressed_id(bsp_key_id_t key_id)
{
#if BSP_KEY_ACTIVE_LOW
    return (bsp_key_read_id(key_id) == 0U) ? 1U : 0U;
#else
    return (bsp_key_read_id(key_id) != 0U) ? 1U : 0U;
#endif
}

/*
 * 扫描指定按键的消抖事件。
 * 参数 key_id 指定按键编号；返回按下、释放或无事件。函数需要在主循环中周期调用。
 */
bsp_key_event_t bsp_key_scan_event_id(bsp_key_id_t key_id)
{
    uint8_t pressed;
    uint32_t current_tick;

    if (bsp_key_id_is_valid(key_id) == 0U) {
        return BSP_KEY_EVENT_NONE;
    }

    current_tick = systick_get_tick();
    pressed = bsp_key_is_pressed_id(key_id);

    /* 采样值变化时先更新时间戳，等待消抖时间结束后才确认稳定事件。 */
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

/*
 * 判断指定按键是否刚刚产生按下事件。
 * 参数 key_id 指定按键编号；返回 1 表示检测到一次新的按下事件，返回 0 表示没有。
 */
uint8_t bsp_key_was_pressed_id(bsp_key_id_t key_id)
{
    return (bsp_key_scan_event_id(key_id) == BSP_KEY_EVENT_PRESSED) ? 1U : 0U;
}

/*
 * 读取默认 KEY1 的原始电平。
 * 无传入参数；返回 1 表示高电平，0 表示低电平。
 */
uint8_t bsp_key_read(void)
{
    return bsp_key_read_id(BSP_KEY_ID_1);
}

/*
 * 判断默认 KEY1 是否按下。
 * 无传入参数；返回 1 表示按下，0 表示未按下。
 */
uint8_t bsp_key_is_pressed(void)
{
    return bsp_key_is_pressed_id(BSP_KEY_ID_1);
}

/*
 * 扫描默认 KEY1 的消抖事件。
 * 无传入参数；返回按下、释放或无事件。
 */
bsp_key_event_t bsp_key_scan_event(void)
{
    return bsp_key_scan_event_id(BSP_KEY_ID_1);
}

/*
 * 判断默认 KEY1 是否刚刚按下。
 * 无传入参数；返回 1 表示出现新的按下事件，0 表示没有。
 */
uint8_t bsp_key_was_pressed(void)
{
    return bsp_key_was_pressed_id(BSP_KEY_ID_1);
}

/*
 * KEY1 外部中断服务入口。
 * 无传入参数，无返回值；这里只清除 EXTI 标志，具体按键事件仍由主循环消抖扫描确认。
 */
void bsp_key_exti_irq_handler(void)
{
    if (exti_interrupt_flag_get(BSP_KEY_EXTI_LINE) == RESET) {
        return;
    }

    exti_interrupt_flag_clear(BSP_KEY_EXTI_LINE);
}
