#include "bsp_led.h"

/*
 * 初始化系统状态灯和采集状态灯。
 * 无传入参数，无返回值；函数会配置两个 GPIO 为推挽输出，并默认全部熄灭。
 */
void bsp_led_init(void)
{
    rcu_periph_clock_enable(BSP_LED_RCU);
    rcu_periph_clock_enable(BSP_LED_SAMPLE_RCU);

    gpio_mode_set(BSP_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BSP_LED_PIN);
    gpio_output_options_set(BSP_LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_LED_PIN);
    gpio_mode_set(BSP_LED_SAMPLE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BSP_LED_SAMPLE_PIN);
    gpio_output_options_set(BSP_LED_SAMPLE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_LED_SAMPLE_PIN);

    bsp_led_off();
    bsp_led_sample_off();
}

/*
 * 点亮系统状态灯。
 * 无传入参数，无返回值；具体高电平/低电平点亮由 BSP_LED_ACTIVE_HIGH 宏决定。
 */
void bsp_led_on(void)
{
#if BSP_LED_ACTIVE_HIGH
    gpio_bit_set(BSP_LED_PORT, BSP_LED_PIN);
#else
    gpio_bit_reset(BSP_LED_PORT, BSP_LED_PIN);
#endif
}

/*
 * 熄灭系统状态灯。
 * 无传入参数，无返回值；用于 App 空闲状态、睡眠前外设关闭等场景。
 */
void bsp_led_off(void)
{
#if BSP_LED_ACTIVE_HIGH
    gpio_bit_reset(BSP_LED_PORT, BSP_LED_PIN);
#else
    gpio_bit_set(BSP_LED_PORT, BSP_LED_PIN);
#endif
}

/*
 * 翻转系统状态灯。
 * 无传入参数，无返回值；App 运行后每 1 秒调用一次用于状态指示。
 */
void bsp_led_toggle(void)
{
    gpio_bit_toggle(BSP_LED_PORT, BSP_LED_PIN);
}

/*
 * 点亮采集状态灯。
 * 无传入参数，无返回值；自动采集上报期间保持常亮。
 */
void bsp_led_sample_on(void)
{
#if BSP_LED_SAMPLE_ACTIVE_HIGH
    gpio_bit_set(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#else
    gpio_bit_reset(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#endif
}

/*
 * 熄灭采集状态灯。
 * 无传入参数，无返回值；自动采集停止或进入睡眠时调用。
 */
void bsp_led_sample_off(void)
{
#if BSP_LED_SAMPLE_ACTIVE_HIGH
    gpio_bit_reset(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#else
    gpio_bit_set(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#endif
}

/*
 * 翻转采集状态灯。
 * 无传入参数，无返回值；预留给调试或后续扩展使用。
 */
void bsp_led_sample_toggle(void)
{
    gpio_bit_toggle(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
}
