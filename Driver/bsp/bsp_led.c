#include "bsp_led.h"

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

void bsp_led_on(void)
{
#if BSP_LED_ACTIVE_HIGH
    gpio_bit_set(BSP_LED_PORT, BSP_LED_PIN);
#else
    gpio_bit_reset(BSP_LED_PORT, BSP_LED_PIN);
#endif
}

void bsp_led_off(void)
{
#if BSP_LED_ACTIVE_HIGH
    gpio_bit_reset(BSP_LED_PORT, BSP_LED_PIN);
#else
    gpio_bit_set(BSP_LED_PORT, BSP_LED_PIN);
#endif
}

void bsp_led_toggle(void)
{
    gpio_bit_toggle(BSP_LED_PORT, BSP_LED_PIN);
}

void bsp_led_sample_on(void)
{
#if BSP_LED_SAMPLE_ACTIVE_HIGH
    gpio_bit_set(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#else
    gpio_bit_reset(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#endif
}

void bsp_led_sample_off(void)
{
#if BSP_LED_SAMPLE_ACTIVE_HIGH
    gpio_bit_reset(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#else
    gpio_bit_set(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
#endif
}

void bsp_led_sample_toggle(void)
{
    gpio_bit_toggle(BSP_LED_SAMPLE_PORT, BSP_LED_SAMPLE_PIN);
}
