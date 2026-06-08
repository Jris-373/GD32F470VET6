#include "bsp_adc.h"

/*
 * 根据业务通道编号换算到 GD32 ADC 物理通道。
 * 参数 input 为业务层使用的 CH0/CH1，channel 为输出的 ADC 通道号指针。
 * 返回 1 表示通道有效，返回 0 表示参数为空或通道不存在。
 */
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

/*
 * 初始化板载 ADC 采样资源。
 * 无传入参数，无返回值；函数会打开 GPIO/ADC 时钟，将 CH0/CH1 引脚设为模拟输入，
 * 并配置 12 位单次转换模式。业务层读取电位器和 DAC 回读前必须先调用该函数。
 */
void bsp_adc_init(void)
{
    rcu_periph_clock_enable(BSP_ADC_CH0_GPIO_RCU);
    rcu_periph_clock_enable(BSP_ADC_CH1_GPIO_RCU);
    rcu_periph_clock_enable(BSP_ADC_RCU);

    gpio_mode_set(BSP_ADC_CH0_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_ADC_CH0_GPIO_PIN);
    gpio_mode_set(BSP_ADC_CH1_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_ADC_CH1_GPIO_PIN);

    /* ADC 采用较低时钟和较长采样时间，提高电位器/DAC 回读的稳定性。 */
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

/*
 * 读取指定业务通道的原始 ADC 值。
 * 参数 input 指定 CH0 或 CH1；返回 0~4095 的 12 位采样值，读取失败时返回 0。
 */
uint16_t bsp_adc_read_input_raw(bsp_adc_input_t input)
{
    uint16_t raw = 0U;

    (void)bsp_adc_read_input_raw_timeout(input, &raw, 0U);
    return raw;
}

/*
 * 带超时保护地读取指定业务通道的原始 ADC 值。
 * 参数 input 指定 CH0/CH1，raw 用于返回采样值，timeout 为轮询等待上限；timeout 为 0 表示一直等待。
 * 返回 1 表示读取成功，返回 0 表示参数错误、通道无效或等待超时。
 */
uint8_t bsp_adc_read_input_raw_timeout(bsp_adc_input_t input, uint16_t *raw, uint32_t timeout)
{
    uint8_t channel;

    if ((raw == 0) || (bsp_adc_input_channel(input, &channel) == 0U)) {
        return 0U;
    }

    /* 每次读取前切换例行通道，保证 CH0/CH1 可以共用同一个 ADC 外设。 */
    adc_routine_channel_config(BSP_ADC_PERIPH, 0U, channel, ADC_SAMPLETIME_480);
    adc_flag_clear(BSP_ADC_PERIPH, ADC_FLAG_EOC);
    adc_software_trigger_enable(BSP_ADC_PERIPH, ADC_ROUTINE_CHANNEL);

    /* 等待转换完成；带超时版本避免硬件异常时卡死主循环。 */
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

/*
 * 读取默认 CH0 的原始 ADC 值。
 * 无传入参数；返回 0~4095 的 12 位采样值。
 */
uint16_t bsp_adc_read_raw(void)
{
    return bsp_adc_read_input_raw(BSP_ADC_INPUT_CH0);
}

/*
 * 带超时保护地读取默认 CH0 的原始 ADC 值。
 * 参数 raw 返回采样值，timeout 为轮询等待上限；返回 1 表示成功，0 表示失败。
 */
uint8_t bsp_adc_read_raw_timeout(uint16_t *raw, uint32_t timeout)
{
    return bsp_adc_read_input_raw_timeout(BSP_ADC_INPUT_CH0, raw, timeout);
}

/*
 * 读取默认 CH0 并换算为毫伏。
 * 无传入参数；返回基于 BSP_ADC_VREF_MV 计算出的电压值。
 */
uint16_t bsp_adc_read_mv(void)
{
    return bsp_adc_raw_to_mv(bsp_adc_read_raw());
}

/*
 * 带超时保护地读取默认 CH0，并换算为毫伏。
 * 参数 millivolt 返回电压值，timeout 为轮询等待上限；返回 1 表示成功，0 表示失败。
 */
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

/*
 * 将 12 位 ADC 原始值换算为毫伏。
 * 参数 raw 为 0~4095 的采样值，超过满量程会被钳位；返回对应毫伏值。
 */
uint16_t bsp_adc_raw_to_mv(uint16_t raw)
{
    if (raw > BSP_ADC_MAX_RAW) {
        raw = BSP_ADC_MAX_RAW;
    }

    return (uint16_t)(((uint32_t)raw * BSP_ADC_VREF_MV) / BSP_ADC_MAX_RAW);
}
