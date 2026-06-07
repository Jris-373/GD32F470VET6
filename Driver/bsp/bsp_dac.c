#include "bsp_dac.h"

static uint16_t s_dac_raw;

void bsp_dac_init(void)
{
    rcu_periph_clock_enable(BSP_DAC_GPIO_RCU);
    rcu_periph_clock_enable(BSP_DAC_RCU);

    gpio_mode_set(BSP_DAC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_DAC_GPIO_PIN);

    dac_deinit(BSP_DAC_PERIPH);
    dac_trigger_disable(BSP_DAC_PERIPH, BSP_DAC_OUT);
    dac_wave_mode_config(BSP_DAC_PERIPH, BSP_DAC_OUT, DAC_WAVE_DISABLE);
    dac_output_buffer_enable(BSP_DAC_PERIPH, BSP_DAC_OUT);
    bsp_dac_write_raw(0U);
    dac_enable(BSP_DAC_PERIPH, BSP_DAC_OUT);
}

void bsp_dac_write_raw(uint16_t raw)
{
    if (raw > BSP_DAC_MAX_RAW) {
        raw = BSP_DAC_MAX_RAW;
    }

    s_dac_raw = raw;
    dac_data_set(BSP_DAC_PERIPH, BSP_DAC_OUT, DAC_ALIGN_12B_R, raw);
}

void bsp_dac_write_mv(uint16_t millivolt)
{
    bsp_dac_write_raw(bsp_dac_mv_to_raw(millivolt));
}

void bsp_dac_write_percent(uint8_t percent)
{
    uint32_t raw;

    if (percent > 100U) {
        percent = 100U;
    }

    raw = ((uint32_t)percent * BSP_DAC_MAX_RAW) / 100U;
    bsp_dac_write_raw((uint16_t)raw);
}

uint16_t bsp_dac_get_raw(void)
{
    return s_dac_raw;
}

uint16_t bsp_dac_get_mv(void)
{
    return bsp_dac_raw_to_mv(s_dac_raw);
}

uint16_t bsp_dac_mv_to_raw(uint16_t millivolt)
{
    uint32_t raw;

    if (millivolt > BSP_DAC_VREF_MV) {
        millivolt = BSP_DAC_VREF_MV;
    }

    raw = ((uint32_t)millivolt * BSP_DAC_MAX_RAW) / BSP_DAC_VREF_MV;
    return (uint16_t)raw;
}

uint16_t bsp_dac_raw_to_mv(uint16_t raw)
{
    if (raw > BSP_DAC_MAX_RAW) {
        raw = BSP_DAC_MAX_RAW;
    }

    return (uint16_t)(((uint32_t)raw * BSP_DAC_VREF_MV) / BSP_DAC_MAX_RAW);
}
