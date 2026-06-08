#include "bsp_dac.h"

static uint16_t s_dac_raw;

/*
 * 初始化 DAC 输出通道。
 * 无传入参数，无返回值；函数会打开 DAC 和 GPIO 时钟，将输出引脚配置为模拟模式，
 * 并默认输出 0，供赛题 0x0301 设置 DAC 原始值时使用。
 */
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

/*
 * 写入 DAC 原始值。
 * 参数 raw 为 0~4095 的 12 位数值，超过满量程会自动钳位；无返回值。
 */
void bsp_dac_write_raw(uint16_t raw)
{
    if (raw > BSP_DAC_MAX_RAW) {
        raw = BSP_DAC_MAX_RAW;
    }

    s_dac_raw = raw;
    dac_data_set(BSP_DAC_PERIPH, BSP_DAC_OUT, DAC_ALIGN_12B_R, raw);
}

/*
 * 按毫伏写入 DAC 输出。
 * 参数 millivolt 为目标电压毫伏值，函数内部会先换算成 12 位 raw 后输出；无返回值。
 */
void bsp_dac_write_mv(uint16_t millivolt)
{
    bsp_dac_write_raw(bsp_dac_mv_to_raw(millivolt));
}

/*
 * 按百分比写入 DAC 输出。
 * 参数 percent 为 0~100 的输出比例，超过 100 会被钳位；无返回值。
 */
void bsp_dac_write_percent(uint8_t percent)
{
    uint32_t raw;

    if (percent > 100U) {
        percent = 100U;
    }

    raw = ((uint32_t)percent * BSP_DAC_MAX_RAW) / 100U;
    bsp_dac_write_raw((uint16_t)raw);
}

/*
 * 获取最近一次写入的 DAC 原始值。
 * 无传入参数；返回软件缓存的 0~4095 原始值。
 */
uint16_t bsp_dac_get_raw(void)
{
    return s_dac_raw;
}

/*
 * 获取最近一次 DAC 输出对应的毫伏值。
 * 无传入参数；返回由软件缓存 raw 换算出的电压。
 */
uint16_t bsp_dac_get_mv(void)
{
    return bsp_dac_raw_to_mv(s_dac_raw);
}

/*
 * 将目标毫伏值换算为 DAC 原始值。
 * 参数 millivolt 为目标电压，超过参考电压会被钳位；返回 0~4095 的 raw。
 */
uint16_t bsp_dac_mv_to_raw(uint16_t millivolt)
{
    uint32_t raw;

    if (millivolt > BSP_DAC_VREF_MV) {
        millivolt = BSP_DAC_VREF_MV;
    }

    raw = ((uint32_t)millivolt * BSP_DAC_MAX_RAW) / BSP_DAC_VREF_MV;
    return (uint16_t)raw;
}

/*
 * 将 DAC 原始值换算为毫伏。
 * 参数 raw 为 0~4095，超过满量程会被钳位；返回按 BSP_DAC_VREF_MV 计算的电压。
 */
uint16_t bsp_dac_raw_to_mv(uint16_t raw)
{
    if (raw > BSP_DAC_MAX_RAW) {
        raw = BSP_DAC_MAX_RAW;
    }

    return (uint16_t)(((uint32_t)raw * BSP_DAC_VREF_MV) / BSP_DAC_MAX_RAW);
}
