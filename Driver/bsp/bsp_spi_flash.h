#ifndef BSP_SPI_FLASH_H
#define BSP_SPI_FLASH_H

#include "gd32f4xx.h"

#define BSP_SPI_FLASH_PERIPH       SPI0
#define BSP_SPI_FLASH_RCU          RCU_SPI0
#define BSP_SPI_FLASH_GPIO_AF      GPIO_AF_5
#define BSP_SPI_FLASH_SCK_RCU      RCU_GPIOB
#define BSP_SPI_FLASH_SCK_PORT     GPIOB
#define BSP_SPI_FLASH_SCK_PIN      GPIO_PIN_3
#define BSP_SPI_FLASH_MISO_RCU     RCU_GPIOB
#define BSP_SPI_FLASH_MISO_PORT    GPIOB
#define BSP_SPI_FLASH_MISO_PIN     GPIO_PIN_4
#define BSP_SPI_FLASH_MOSI_RCU     RCU_GPIOB
#define BSP_SPI_FLASH_MOSI_PORT    GPIOB
#define BSP_SPI_FLASH_MOSI_PIN     GPIO_PIN_5
#define BSP_SPI_FLASH_CS_RCU       RCU_GPIOA
#define BSP_SPI_FLASH_CS_PORT      GPIOA
#define BSP_SPI_FLASH_CS_PIN       GPIO_PIN_15
#define BSP_SPI_FLASH_PAGE_SIZE    256U
#define BSP_SPI_FLASH_SECTOR_SIZE  4096U
#define BSP_SPI_FLASH_TIMEOUT      1000000U

void bsp_spi_flash_init(void);
uint32_t bsp_spi_flash_read_id(void);
uint8_t bsp_spi_flash_read(uint32_t address, uint8_t *data, uint32_t length);
uint8_t bsp_spi_flash_page_program(uint32_t address, const uint8_t *data, uint16_t length);
uint8_t bsp_spi_flash_write(uint32_t address, const uint8_t *data, uint32_t length);
uint8_t bsp_spi_flash_sector_erase(uint32_t address);
uint8_t bsp_spi_flash_chip_erase(void);

#endif /* BSP_SPI_FLASH_H */
