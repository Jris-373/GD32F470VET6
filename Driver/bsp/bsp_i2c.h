#ifndef BSP_I2C_H
#define BSP_I2C_H

#include "gd32f4xx.h"

#define BSP_I2C0_PERIPH          I2C0
#define BSP_I2C0_RCU             RCU_I2C0
#define BSP_I2C0_SCL_RCU         RCU_GPIOB
#define BSP_I2C0_SCL_PORT        GPIOB
#define BSP_I2C0_SCL_PIN         GPIO_PIN_8
#define BSP_I2C0_SDA_RCU         RCU_GPIOB
#define BSP_I2C0_SDA_PORT        GPIOB
#define BSP_I2C0_SDA_PIN         GPIO_PIN_9
#define BSP_I2C0_GPIO_AF         GPIO_AF_4
#define BSP_I2C0_SPEED_HZ        400000U
#define BSP_I2C_TIMEOUT_DEFAULT  100000U

void bsp_i2c0_init(void);
uint8_t bsp_i2c0_is_device_ready(uint8_t address_7bit, uint32_t timeout);
uint8_t bsp_i2c0_write(uint8_t address_7bit, const uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c0_read(uint8_t address_7bit, uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c0_write_reg(uint8_t address_7bit, uint8_t reg, const uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c0_read_reg(uint8_t address_7bit, uint8_t reg, uint8_t *data, uint16_t length, uint32_t timeout);

#endif /* BSP_I2C_H */
