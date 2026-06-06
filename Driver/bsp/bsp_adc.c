#include "bsp_adc.h"

static uint8_t bsp_adc_input_channel(bsp_adc_input_t input, uint8_t *channel)
{
    if (channel == 0) {
        return 0U;
    }

    switch (input) {
    case BSP_ADC_INPUT_CH0:
        *channel = BSP_ADC_CH0_CHANNEL;
        return 1U;

    case BSP_ADC_INPUT_CH1:
        *channel = BSP_ADC_CH1_CHANNEL;
        return 1U;

    default:
        return 0U;
    }
}

void bsp_adc_init(void)
{
    rcu_periph_clock_enable(BSP_ADC_CH0_GPIO_RCU);
    rcu_periph_clock_enable(BSP_ADC_CH1_GPIO_RCU);
    rcu_periph_clock_enable(BSP_ADC_RCU);

    gpio_mode_set(BSP_ADC_CH0_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_ADC_CH0_GPIO_PIN);
    gpio_mode_set(BSP_ADC_CH1_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_ADC_CH1_GPIO_PIN);

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_deinit();
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_resolution_config(BSP_ADC_PERIPH, ADC_RESOLUTION_12B);
    adc_data_alignment_config(BSP_ADC_PERIPH, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(BSP_ADC_PERIPH, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(BSP_ADC_PERIPH, ADC_CONTINUOUS_MODE, DISABLE);
    adc_channel_length_config(BSP_ADC_PERIPH, ADC_ROUTINE_CHANNEL, 1U);
    adc_routine_channel_config(BSP_ADC_PERIPH, 0U, BSP_ADC_CH0_CHANNEL, ADC_SAMPLETIME_480);
    adc_external_trigger_config(BSP_ADC_PERIPH, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(BSP_ADC_PERIPH);
    adc_calibration_enable(BSP_ADC_PERIPH);
}

uint16_t bsp_adc_read_input_raw(bsp_adc_input_t input)
{
    uint16_t raw = 0U;

    (void)bsp_adc_read_input_raw_timeout(input, &raw, 0U);
    return raw;
}

uint8_t bsp_adc_read_input_raw_timeout(bsp_adc_input_t input, uint16_t *raw, uint32_t timeout)
{
    uint8_t channel;

    if ((raw == 0) || (bsp_adc_input_channel(input, &channel) == 0U)) {
        return 0U;
    }

    adc_routine_channel_config(BSP_ADC_PERIPH, 0U, channel, ADC_SAMPLETIME_480);
    adc_flag_clear(BSP_ADC_PERIPH, ADC_FLAG_EOC);
    adc_software_trigger_enable(BSP_ADC_PERIPH, ADC_ROUTINE_CHANNEL);

    while (adc_flag_get(BSP_ADC_PERIPH, ADC_FLAG_EOC) == RESET) {
        if (timeout != 0U) {
            timeout--;
            if (timeout == 0U) {
                return 0U;
            }
        }
    }

    *raw = adc_routine_data_read(BSP_ADC_PERIPH);
    return 1U;
}

uint16_t bsp_adc_read_raw(void)
{
    return bsp_adc_read_input_raw(BSP_ADC_INPUT_CH0);
}

uint8_t bsp_adc_read_raw_timeout(uint16_t *raw, uint32_t timeout)
{
    return bsp_adc_read_input_raw_timeout(BSP_ADC_INPUT_CH0, raw, timeout);
}

uint16_t bsp_adc_read_mv(void)
{
    return bsp_adc_raw_to_mv(bsp_adc_read_raw());
}

uint8_t bsp_adc_read_mv_timeout(uint16_t *millivolt, uint32_t timeout)
{
    uint16_t raw;

    if (millivolt == 0) {
        return 0U;
    }

    if (bsp_adc_read_raw_timeout(&raw, timeout) == 0U) {
        return 0U;
    }

    *millivolt = bsp_adc_raw_to_mv(raw);
    return 1U;
}

uint16_t bsp_adc_raw_to_mv(uint16_t raw)
{
    if (raw > BSP_ADC_MAX_RAW) {
        raw = BSP_ADC_MAX_RAW;
    }

    return (uint16_t)(((uint32_t)raw * BSP_ADC_VREF_MV) / BSP_ADC_MAX_RAW);
}
