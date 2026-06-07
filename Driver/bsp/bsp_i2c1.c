#include "bsp_i2c1.h"

static uint32_t bsp_i2c1_timeout_limit(uint32_t timeout)
{
    return (timeout == 0U) ? BSP_I2C1_TIMEOUT_DEFAULT : timeout;
}

static uint8_t bsp_i2c1_wait_flag(i2c_flag_enum flag, FlagStatus status, uint32_t timeout)
{
    timeout = bsp_i2c1_timeout_limit(timeout);

    while (i2c_flag_get(BSP_I2C1_PERIPH, flag) != status) {
        timeout--;
        if (timeout == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t bsp_i2c1_wait_stop(uint32_t timeout)
{
    timeout = bsp_i2c1_timeout_limit(timeout);

    while ((I2C_CTL0(BSP_I2C1_PERIPH) & I2C_CTL0_STOP) != 0U) {
        timeout--;
        if (timeout == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t bsp_i2c1_send_start_and_address(uint8_t address_7bit, uint32_t direction, uint8_t wait_busy, uint8_t clear_address, uint32_t timeout)
{
    if ((wait_busy != 0U) && (bsp_i2c1_wait_flag(I2C_FLAG_I2CBSY, RESET, timeout) == 0U)) {
        return 0U;
    }

    i2c_start_on_bus(BSP_I2C1_PERIPH);
    if (bsp_i2c1_wait_flag(I2C_FLAG_SBSEND, SET, timeout) == 0U) {
        return 0U;
    }

    i2c_master_addressing(BSP_I2C1_PERIPH, ((uint32_t)address_7bit << 1U), direction);
    if (bsp_i2c1_wait_flag(I2C_FLAG_ADDSEND, SET, timeout) == 0U) {
        return 0U;
    }

    if (clear_address != 0U) {
        i2c_flag_clear(BSP_I2C1_PERIPH, I2C_FLAG_ADDSEND);
    }

    return 1U;
}

void bsp_i2c1_init(void)
{
    rcu_periph_clock_enable(BSP_I2C1_SCL_RCU);
    rcu_periph_clock_enable(BSP_I2C1_SDA_RCU);
    rcu_periph_clock_enable(BSP_I2C1_RCU);

    gpio_af_set(BSP_I2C1_SCL_PORT, BSP_I2C1_GPIO_AF, BSP_I2C1_SCL_PIN);
    gpio_af_set(BSP_I2C1_SDA_PORT, BSP_I2C1_GPIO_AF, BSP_I2C1_SDA_PIN);

    gpio_mode_set(BSP_I2C1_SCL_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BSP_I2C1_SCL_PIN);
    gpio_output_options_set(BSP_I2C1_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, BSP_I2C1_SCL_PIN);

    gpio_mode_set(BSP_I2C1_SDA_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BSP_I2C1_SDA_PIN);
    gpio_output_options_set(BSP_I2C1_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, BSP_I2C1_SDA_PIN);

    i2c_deinit(BSP_I2C1_PERIPH);
    i2c_clock_config(BSP_I2C1_PERIPH, BSP_I2C1_SPEED_HZ, I2C_DTCY_2);
    i2c_mode_addr_config(BSP_I2C1_PERIPH, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x00U);
    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    i2c_enable(BSP_I2C1_PERIPH);
}

uint8_t bsp_i2c1_is_device_ready(uint8_t address_7bit, uint32_t timeout)
{
    uint8_t ready;

    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    ready = bsp_i2c1_send_start_and_address(address_7bit, I2C_TRANSMITTER, 1U, 1U, timeout);
    i2c_stop_on_bus(BSP_I2C1_PERIPH);
    (void)bsp_i2c1_wait_stop(timeout);

    return ready;
}

uint8_t bsp_i2c1_write(uint8_t address_7bit, const uint8_t *data, uint16_t length, uint32_t timeout)
{
    uint16_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    if (bsp_i2c1_send_start_and_address(address_7bit, I2C_TRANSMITTER, 1U, 1U, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }

    for (index = 0U; index < length; index++) {
        if (bsp_i2c1_wait_flag(I2C_FLAG_TBE, SET, timeout) == 0U) {
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }
        i2c_data_transmit(BSP_I2C1_PERIPH, data[index]);
    }

    if (bsp_i2c1_wait_flag(I2C_FLAG_BTC, SET, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }

    i2c_stop_on_bus(BSP_I2C1_PERIPH);
    return bsp_i2c1_wait_stop(timeout);
}

uint8_t bsp_i2c1_read(uint8_t address_7bit, uint8_t *data, uint16_t length, uint32_t timeout)
{
    uint16_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    if (length == 0U) {
        return 1U;
    }

    if (length == 1U) {
        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
        if (bsp_i2c1_send_start_and_address(address_7bit, I2C_RECEIVER, 1U, 0U, timeout) == 0U) {
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }

        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_DISABLE);
        i2c_flag_clear(BSP_I2C1_PERIPH, I2C_FLAG_ADDSEND);
        i2c_stop_on_bus(BSP_I2C1_PERIPH);

        if (bsp_i2c1_wait_flag(I2C_FLAG_RBNE, SET, timeout) == 0U) {
            i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
            return 0U;
        }

        data[0] = (uint8_t)i2c_data_receive(BSP_I2C1_PERIPH);
        (void)bsp_i2c1_wait_stop(timeout);
        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
        return 1U;
    }

    if (length == 2U) {
        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
        if (bsp_i2c1_send_start_and_address(address_7bit, I2C_RECEIVER, 1U, 1U, timeout) == 0U) {
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }

        if (bsp_i2c1_wait_flag(I2C_FLAG_RBNE, SET, timeout) == 0U) {
            i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }
        data[0] = (uint8_t)i2c_data_receive(BSP_I2C1_PERIPH);
        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_DISABLE);

        if (bsp_i2c1_wait_flag(I2C_FLAG_RBNE, SET, timeout) == 0U) {
            i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }
        data[1] = (uint8_t)i2c_data_receive(BSP_I2C1_PERIPH);
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        (void)bsp_i2c1_wait_stop(timeout);
        i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
        return 1U;
    }

    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    if (bsp_i2c1_send_start_and_address(address_7bit, I2C_RECEIVER, 1U, 1U, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }

    for (index = 0U; index < length; index++) {
        if (index == (uint16_t)(length - 1U)) {
            i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_DISABLE);
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
        }

        if (bsp_i2c1_wait_flag(I2C_FLAG_RBNE, SET, timeout) == 0U) {
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
            return 0U;
        }

        data[index] = (uint8_t)i2c_data_receive(BSP_I2C1_PERIPH);
    }

    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    return bsp_i2c1_wait_stop(timeout);
}

uint8_t bsp_i2c1_write_reg(uint8_t address_7bit, uint8_t reg, const uint8_t *data, uint16_t length, uint32_t timeout)
{
    uint16_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    i2c_ack_config(BSP_I2C1_PERIPH, I2C_ACK_ENABLE);
    if (bsp_i2c1_send_start_and_address(address_7bit, I2C_TRANSMITTER, 1U, 1U, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }

    if (bsp_i2c1_wait_flag(I2C_FLAG_TBE, SET, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }
    i2c_data_transmit(BSP_I2C1_PERIPH, reg);

    for (index = 0U; index < length; index++) {
        if (bsp_i2c1_wait_flag(I2C_FLAG_TBE, SET, timeout) == 0U) {
            i2c_stop_on_bus(BSP_I2C1_PERIPH);
            return 0U;
        }
        i2c_data_transmit(BSP_I2C1_PERIPH, data[index]);
    }

    if (bsp_i2c1_wait_flag(I2C_FLAG_BTC, SET, timeout) == 0U) {
        i2c_stop_on_bus(BSP_I2C1_PERIPH);
        return 0U;
    }

    i2c_stop_on_bus(BSP_I2C1_PERIPH);
    return bsp_i2c1_wait_stop(timeout);
}

uint8_t bsp_i2c1_read_reg(uint8_t address_7bit, uint8_t reg, uint8_t *data, uint16_t length, uint32_t timeout)
{
    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    if (bsp_i2c1_write_reg(address_7bit, reg, 0, 0U, timeout) == 0U) {
        return 0U;
    }

    return bsp_i2c1_read(address_7bit, data, length, timeout);
}
