#include "bsp_pt100.h"
#include "bsp_gd30ad3340.h"

#define BSP_PT100_DEFAULT_V_TO_R_GAIN   1006.2893f
#define BSP_PT100_DEFAULT_V_TO_R_OFFSET 0.0f
#define BSP_PT100_R0_OHMS               100.0f
#define BSP_PT100_CVD_A                 0.0039083f
#define BSP_PT100_CVD_B                 (-0.0000005775f)
#define BSP_PT100_CVD_C                 (-0.000000000004183f)
#define BSP_PT100_MIN_TEMP_C            (-200.0f)
#define BSP_PT100_MAX_TEMP_C            850.0f
#define BSP_PT100_SEARCH_STEPS          32U

volatile uint8_t g_bsp_pt100_debug_error;
volatile float g_bsp_pt100_debug_voltage;
volatile float g_bsp_pt100_debug_resistance;
volatile float g_bsp_pt100_debug_temperature;

static bsp_pt100_status_t s_pt100_status = BSP_PT100_STATUS_NOT_READY;
static bsp_pt100_calibration_t s_pt100_calibration = {
    BSP_PT100_DEFAULT_V_TO_R_GAIN,
    BSP_PT100_DEFAULT_V_TO_R_OFFSET,
};

/*
 * 根据 IEC60751 Callendar-Van Dusen 公式由温度反算 PT100 电阻。
 * 参数 temperature 为摄氏温度；返回该温度下的理论阻值，单位欧姆。
 */
static float bsp_pt100_resistance_from_temperature(float temperature)
{
    float resistance;

    resistance = BSP_PT100_R0_OHMS *
                 (1.0f + BSP_PT100_CVD_A * temperature + BSP_PT100_CVD_B * temperature * temperature);

    if (temperature < 0.0f) {
        resistance += BSP_PT100_R0_OHMS * BSP_PT100_CVD_C * (temperature - 100.0f) * temperature * temperature * temperature;
    }

    return resistance;
}

/*
 * 根据 PT100 阻值反算温度。
 * 参数 resistance 为测得阻值，temperature 用于返回摄氏温度；返回 1 表示成功，0 表示阻值越界或参数错误。
 */
static uint8_t bsp_pt100_temperature_from_resistance(float resistance, float *temperature)
{
    float low;
    float high;
    float middle;
    uint8_t index;

    if ((temperature == 0) || (resistance <= 0.0f)) {
        g_bsp_pt100_debug_error = 2U;
        return 0U;
    }

    if ((resistance < bsp_pt100_resistance_from_temperature(BSP_PT100_MIN_TEMP_C)) ||
        (resistance > bsp_pt100_resistance_from_temperature(BSP_PT100_MAX_TEMP_C))) {
        g_bsp_pt100_debug_error = 3U;
        return 0U;
    }

    /* 公式正向计算更稳定，这里用二分法在 PT100 合法温度范围内反求温度。 */
    low = BSP_PT100_MIN_TEMP_C;
    high = BSP_PT100_MAX_TEMP_C;
    for (index = 0U; index < BSP_PT100_SEARCH_STEPS; index++) {
        middle = (low + high) * 0.5f;
        if (bsp_pt100_resistance_from_temperature(middle) < resistance) {
            low = middle;
        } else {
            high = middle;
        }
    }

    *temperature = (low + high) * 0.5f;
    g_bsp_pt100_debug_temperature = *temperature;
    g_bsp_pt100_debug_error = 0U;
    return 1U;
}

/*
 * 初始化 PT100 采样链路。
 * 无传入参数，无返回值；函数会恢复默认标定参数并初始化 GD30AD3340 差分采样。
 */
void bsp_pt100_init(void)
{
    s_pt100_calibration.voltage_to_resistance_gain = BSP_PT100_DEFAULT_V_TO_R_GAIN;
    s_pt100_calibration.voltage_to_resistance_offset = BSP_PT100_DEFAULT_V_TO_R_OFFSET;
    bsp_gd30ad3340_init();
    if (bsp_gd30ad3340_is_ready() != 0U) {
        (void)bsp_gd30ad3340_config_differential_ain0_ain1();
    }
    s_pt100_status = (bsp_gd30ad3340_is_ready() != 0U) ? BSP_PT100_STATUS_READY : BSP_PT100_STATUS_NOT_READY;
}

/*
 * 获取 PT100 采样模块状态。
 * 无传入参数；返回 READY 或 NOT_READY，用于协议层决定是否回复错误帧。
 */
bsp_pt100_status_t bsp_pt100_get_status(void)
{
    return s_pt100_status;
}

/*
 * 设置 PT100 电压到阻值的线性标定参数。
 * 参数 calibration 包含 gain 和 offset；返回 1 表示参数合法并已保存，0 表示参数为空或超出合理范围。
 */
uint8_t bsp_pt100_set_calibration(const bsp_pt100_calibration_t *calibration)
{
    if (calibration == 0) {
        return 0U;
    }

    /* 用自比较过滤 NaN，同时限制 gain/offset 范围，避免错误参数导致温度换算失控。 */
    if ((calibration->voltage_to_resistance_gain != calibration->voltage_to_resistance_gain) ||
        (calibration->voltage_to_resistance_offset != calibration->voltage_to_resistance_offset) ||
        (calibration->voltage_to_resistance_gain <= 0.0f) ||
        (calibration->voltage_to_resistance_gain > 10000.0f) ||
        (calibration->voltage_to_resistance_offset < -10000.0f) ||
        (calibration->voltage_to_resistance_offset > 10000.0f)) {
        return 0U;
    }

    s_pt100_calibration = *calibration;
    return 1U;
}

/*
 * 读取当前 PT100 标定参数。
 * 参数 calibration 用于返回 gain 和 offset；参数为空时直接返回。
 */
void bsp_pt100_get_calibration(bsp_pt100_calibration_t *calibration)
{
    if (calibration == 0) {
        return;
    }

    *calibration = s_pt100_calibration;
}

/*
 * 读取外部 ADC 原始转换值。
 * 参数 raw 返回 GD30AD3340 的 16 位原始值；返回 1 表示成功，0 表示外部 ADC 未就绪。
 */
uint8_t bsp_pt100_read_raw(uint32_t *raw)
{
    int16_t adc_raw;

    if (raw != 0) {
        *raw = 0U;
    }

    if ((raw == 0) || (bsp_gd30ad3340_read_raw(&adc_raw) == 0U)) {
        s_pt100_status = BSP_PT100_STATUS_NOT_READY;
        return 0U;
    }

    s_pt100_status = BSP_PT100_STATUS_READY;
    *raw = (uint32_t)(uint16_t)adc_raw;
    return 1U;
}

/*
 * 读取 PT100 等效阻值。
 * 参数 resistance 返回换算后的欧姆值；返回 1 表示成功，0 表示 ADC 读取失败。
 */
uint8_t bsp_pt100_read_resistance(float *resistance)
{
    float voltage;

    if (resistance == 0) {
        return 0U;
    }

    if (bsp_gd30ad3340_read_voltage(&voltage) == 0U) {
        s_pt100_status = BSP_PT100_STATUS_NOT_READY;
        g_bsp_pt100_debug_error = 1U;
        return 0U;
    }

    /* 差分接线方向可能导致负电压，计算阻值时只关心 PT100 两端电压幅值。 */
    if (voltage < 0.0f) {
        voltage = -voltage;
    }

    s_pt100_status = BSP_PT100_STATUS_READY;
    g_bsp_pt100_debug_voltage = voltage;
    *resistance = voltage * s_pt100_calibration.voltage_to_resistance_gain +
                  s_pt100_calibration.voltage_to_resistance_offset;
    g_bsp_pt100_debug_resistance = *resistance;
    return 1U;
}

/*
 * 读取 PT100 温度。
 * 参数 temperature 返回摄氏温度 float；返回 1 表示成功，0 表示采样或换算失败。
 */
uint8_t bsp_pt100_read_temperature(float *temperature)
{
    float resistance;

    if (temperature == 0) {
        return 0U;
    }

    if (bsp_pt100_read_resistance(&resistance) == 0U) {
        return 0U;
    }

    return bsp_pt100_temperature_from_resistance(resistance, temperature);
}

/*
 * 读取 PT100 温度并放大 100 倍。
 * 参数 temperature_x100 返回摄氏温度乘以 100 后的整数；返回 1 表示成功，0 表示失败。
 */
uint8_t bsp_pt100_read_temperature_x100(int32_t *temperature_x100)
{
    float temperature;

    if (temperature_x100 != 0) {
        *temperature_x100 = 0;
    }

    if ((temperature_x100 == 0) || (bsp_pt100_read_temperature(&temperature) == 0U)) {
        return 0U;
    }

    /* 正负温度分别四舍五入，避免直接截断造成显示和协议回传偏差。 */
    if (temperature >= 0.0f) {
        *temperature_x100 = (int32_t)(temperature * 100.0f + 0.5f);
    } else {
        *temperature_x100 = (int32_t)(temperature * 100.0f - 0.5f);
    }

    return 1U;
}
