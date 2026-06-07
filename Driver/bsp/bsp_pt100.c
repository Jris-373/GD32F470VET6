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

bsp_pt100_status_t bsp_pt100_get_status(void)
{
    return s_pt100_status;
}

uint8_t bsp_pt100_set_calibration(const bsp_pt100_calibration_t *calibration)
{
    if (calibration == 0) {
        return 0U;
    }

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

void bsp_pt100_get_calibration(bsp_pt100_calibration_t *calibration)
{
    if (calibration == 0) {
        return;
    }

    *calibration = s_pt100_calibration;
}

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

uint8_t bsp_pt100_read_temperature_x100(int32_t *temperature_x100)
{
    float temperature;

    if (temperature_x100 != 0) {
        *temperature_x100 = 0;
    }

    if ((temperature_x100 == 0) || (bsp_pt100_read_temperature(&temperature) == 0U)) {
        return 0U;
    }

    if (temperature >= 0.0f) {
        *temperature_x100 = (int32_t)(temperature * 100.0f + 0.5f);
    } else {
        *temperature_x100 = (int32_t)(temperature * 100.0f - 0.5f);
    }

    return 1U;
}
