#include "bsp_gd30ad3340.h"
#include "bsp_i2c1.h"

#define GD30AD3340_I2C_ADDR       BSP_GD30AD3340_ADDR_GND
#define GD30AD3340_TIMEOUT        100000U
#define GD30AD3340_REG_CONVERSION 0x00U
#define GD30AD3340_REG_CONFIG     0x01U

#define GD30AD3340_CFG_OS_SINGLE     0x8000U
#define GD30AD3340_CFG_MUX_DIFF_0_1  0x0000U
#define GD30AD3340_CFG_MUX_SINGLE_0  0x4000U
#define GD30AD3340_CFG_MUX_SINGLE_1  0x5000U
#define GD30AD3340_CFG_MUX_SINGLE_2  0x6000U
#define GD30AD3340_CFG_MUX_SINGLE_3  0x7000U
#define GD30AD3340_CFG_PGA_0_256V    0x0A00U
#define GD30AD3340_CFG_MODE_CONTIN   0x0000U
#define GD30AD3340_CFG_DR_100SPS     0x0080U
#define GD30AD3340_CFG_CQUE_NONE     0x0003U
#define GD30AD3340_FULL_SCALE_VOLTS  0.256f
#define GD30AD3340_FULL_SCALE_COUNTS 32768.0f

static uint8_t s_gd30ad3340_ready;
static uint16_t s_gd30ad3340_config;

static uint8_t bsp_gd30ad3340_ensure_ready(void)
{
    if (s_gd30ad3340_ready != 0U) {
        return 1U;
    }

    bsp_gd30ad3340_init();
    return s_gd30ad3340_ready;
}

static uint16_t bsp_gd30ad3340_channel_mux(bsp_gd30ad3340_channel_t channel)
{
    switch (channel) {
    case BSP_GD30AD3340_CHANNEL_AIN1:
        return GD30AD3340_CFG_MUX_SINGLE_1;

    case BSP_GD30AD3340_CHANNEL_AIN2:
        return GD30AD3340_CFG_MUX_SINGLE_2;

    case BSP_GD30AD3340_CHANNEL_AIN3:
        return GD30AD3340_CFG_MUX_SINGLE_3;

    case BSP_GD30AD3340_CHANNEL_AIN0:
    default:
        return GD30AD3340_CFG_MUX_SINGLE_0;
    }
}

static uint8_t bsp_gd30ad3340_write_reg16(uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
    return bsp_i2c1_write_reg(GD30AD3340_I2C_ADDR, reg, data, sizeof(data), GD30AD3340_TIMEOUT);
}

static uint8_t bsp_gd30ad3340_read_reg16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];

    if (value == 0) {
        return 0U;
    }

    if (bsp_i2c1_read_reg(GD30AD3340_I2C_ADDR, reg, data, sizeof(data), GD30AD3340_TIMEOUT) == 0U) {
        return 0U;
    }

    *value = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    return 1U;
}

void bsp_gd30ad3340_init(void)
{
    bsp_i2c1_init();
    s_gd30ad3340_ready = 0U;
    s_gd30ad3340_config = (uint16_t)(GD30AD3340_CFG_OS_SINGLE |
                                     GD30AD3340_CFG_MUX_DIFF_0_1 |
                                     GD30AD3340_CFG_PGA_0_256V |
                                     GD30AD3340_CFG_MODE_CONTIN |
                                     GD30AD3340_CFG_DR_100SPS |
                                     GD30AD3340_CFG_CQUE_NONE);

    if (bsp_i2c1_is_device_ready(GD30AD3340_I2C_ADDR, GD30AD3340_TIMEOUT) == 0U) {
        return;
    }

    if (bsp_gd30ad3340_write_reg16(GD30AD3340_REG_CONFIG, s_gd30ad3340_config) == 0U) {
        return;
    }

    s_gd30ad3340_ready = 1U;
}

uint8_t bsp_gd30ad3340_is_ready(void)
{
    return s_gd30ad3340_ready;
}

uint8_t bsp_gd30ad3340_config_differential_ain0_ain1(void)
{
    s_gd30ad3340_config = (uint16_t)(GD30AD3340_CFG_OS_SINGLE |
                                     GD30AD3340_CFG_MUX_DIFF_0_1 |
                                     GD30AD3340_CFG_PGA_0_256V |
                                     GD30AD3340_CFG_MODE_CONTIN |
                                     GD30AD3340_CFG_DR_100SPS |
                                     GD30AD3340_CFG_CQUE_NONE);

    if (bsp_gd30ad3340_write_reg16(GD30AD3340_REG_CONFIG, s_gd30ad3340_config) == 0U) {
        s_gd30ad3340_ready = 0U;
        return 0U;
    }

    s_gd30ad3340_ready = 1U;
    return 1U;
}

uint8_t bsp_gd30ad3340_config_single_ended(bsp_gd30ad3340_channel_t channel)
{
    s_gd30ad3340_config = (uint16_t)(GD30AD3340_CFG_OS_SINGLE |
                                     bsp_gd30ad3340_channel_mux(channel) |
                                     GD30AD3340_CFG_PGA_0_256V |
                                     GD30AD3340_CFG_MODE_CONTIN |
                                     GD30AD3340_CFG_DR_100SPS |
                                     GD30AD3340_CFG_CQUE_NONE);

    if (bsp_gd30ad3340_write_reg16(GD30AD3340_REG_CONFIG, s_gd30ad3340_config) == 0U) {
        s_gd30ad3340_ready = 0U;
        return 0U;
    }

    s_gd30ad3340_ready = 1U;
    return 1U;
}

uint8_t bsp_gd30ad3340_read_raw(int16_t *raw)
{
    uint16_t value;

    if (raw == 0) {
        return 0U;
    }

    if (bsp_gd30ad3340_ensure_ready() == 0U) {
        return 0U;
    }

    if (bsp_gd30ad3340_read_reg16(GD30AD3340_REG_CONVERSION, &value) == 0U) {
        s_gd30ad3340_ready = 0U;
        if (bsp_gd30ad3340_ensure_ready() == 0U) {
            return 0U;
        }

        if (bsp_gd30ad3340_read_reg16(GD30AD3340_REG_CONVERSION, &value) == 0U) {
            s_gd30ad3340_ready = 0U;
            return 0U;
        }
    }

    *raw = (int16_t)value;
    return 1U;
}

uint8_t bsp_gd30ad3340_read_voltage(float *voltage)
{
    int16_t raw;

    if (voltage == 0) {
        return 0U;
    }

    if (bsp_gd30ad3340_read_raw(&raw) == 0U) {
        return 0U;
    }

    *voltage = ((float)raw * GD30AD3340_FULL_SCALE_VOLTS) / GD30AD3340_FULL_SCALE_COUNTS;
    return 1U;
}
