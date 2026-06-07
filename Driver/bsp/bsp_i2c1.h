#ifndef BSP_I2C1_H
#define BSP_I2C1_H

#include "gd32f4xx.h"

#define BSP_I2C1_PERIPH          I2C1
#define BSP_I2C1_RCU             RCU_I2C1
#define BSP_I2C1_SCL_RCU         RCU_GPIOB
#define BSP_I2C1_SCL_PORT        GPIOB
#define BSP_I2C1_SCL_PIN         GPIO_PIN_10
#define BSP_I2C1_SDA_RCU         RCU_GPIOB
#define BSP_I2C1_SDA_PORT        GPIOB
#define BSP_I2C1_SDA_PIN         GPIO_PIN_11
#define BSP_I2C1_GPIO_AF         GPIO_AF_4
#define BSP_I2C1_SPEED_HZ        100000U
#define BSP_I2C1_TIMEOUT_DEFAULT 100000U

void bsp_i2c1_init(void);
uint8_t bsp_i2c1_is_device_ready(uint8_t address_7bit, uint32_t timeout);
uint8_t bsp_i2c1_write(uint8_t address_7bit, const uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c1_read(uint8_t address_7bit, uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c1_write_reg(uint8_t address_7bit, uint8_t reg, const uint8_t *data, uint16_t length, uint32_t timeout);
uint8_t bsp_i2c1_read_reg(uint8_t address_7bit, uint8_t reg, uint8_t *data, uint16_t length, uint32_t timeout);

#endif /* BSP_I2C1_H */
