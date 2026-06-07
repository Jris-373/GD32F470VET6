#include "bsp_spi_flash.h"

#define SPI_FLASH_CMD_WRITE_ENABLE  0x06U
#define SPI_FLASH_CMD_READ_STATUS   0x05U
#define SPI_FLASH_CMD_READ_ID       0x9FU
#define SPI_FLASH_CMD_READ_DATA     0x03U
#define SPI_FLASH_CMD_PAGE_PROGRAM  0x02U
#define SPI_FLASH_CMD_SECTOR_ERASE  0x20U
#define SPI_FLASH_CMD_CHIP_ERASE    0xC7U
#define SPI_FLASH_STATUS_BUSY       0x01U

static void bsp_spi_flash_cs_low(void)
{
    gpio_bit_reset(BSP_SPI_FLASH_CS_PORT, BSP_SPI_FLASH_CS_PIN);
}

static void bsp_spi_flash_cs_high(void)
{
    gpio_bit_set(BSP_SPI_FLASH_CS_PORT, BSP_SPI_FLASH_CS_PIN);
}

static uint8_t bsp_spi_flash_transfer(uint8_t data)
{
    while (spi_i2s_flag_get(BSP_SPI_FLASH_PERIPH, SPI_FLAG_TBE) == RESET) {
    }

    spi_i2s_data_transmit(BSP_SPI_FLASH_PERIPH, data);

    while (spi_i2s_flag_get(BSP_SPI_FLASH_PERIPH, SPI_FLAG_RBNE) == RESET) {
    }

    return (uint8_t)spi_i2s_data_receive(BSP_SPI_FLASH_PERIPH);
}

static void bsp_spi_flash_send_address(uint32_t address)
{
    (void)bsp_spi_flash_transfer((uint8_t)(address >> 16U));
    (void)bsp_spi_flash_transfer((uint8_t)(address >> 8U));
    (void)bsp_spi_flash_transfer((uint8_t)address);
}

static uint8_t bsp_spi_flash_read_status(void)
{
    uint8_t status;

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_READ_STATUS);
    status = bsp_spi_flash_transfer(0xFFU);
    bsp_spi_flash_cs_high();

    return status;
}

static uint8_t bsp_spi_flash_wait_ready(uint32_t timeout)
{
    while ((bsp_spi_flash_read_status() & SPI_FLASH_STATUS_BUSY) != 0U) {
        timeout--;
        if (timeout == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static void bsp_spi_flash_write_enable(void)
{
    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_WRITE_ENABLE);
    bsp_spi_flash_cs_high();
}

void bsp_spi_flash_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(BSP_SPI_FLASH_SCK_RCU);
    rcu_periph_clock_enable(BSP_SPI_FLASH_MISO_RCU);
    rcu_periph_clock_enable(BSP_SPI_FLASH_MOSI_RCU);
    rcu_periph_clock_enable(BSP_SPI_FLASH_CS_RCU);
    rcu_periph_clock_enable(BSP_SPI_FLASH_RCU);

    gpio_af_set(BSP_SPI_FLASH_SCK_PORT, BSP_SPI_FLASH_GPIO_AF, BSP_SPI_FLASH_SCK_PIN);
    gpio_af_set(BSP_SPI_FLASH_MISO_PORT, BSP_SPI_FLASH_GPIO_AF, BSP_SPI_FLASH_MISO_PIN);
    gpio_af_set(BSP_SPI_FLASH_MOSI_PORT, BSP_SPI_FLASH_GPIO_AF, BSP_SPI_FLASH_MOSI_PIN);

    gpio_mode_set(BSP_SPI_FLASH_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_SPI_FLASH_SCK_PIN);
    gpio_output_options_set(BSP_SPI_FLASH_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_SPI_FLASH_SCK_PIN);

    gpio_mode_set(BSP_SPI_FLASH_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_SPI_FLASH_MISO_PIN);
    gpio_output_options_set(BSP_SPI_FLASH_MISO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_SPI_FLASH_MISO_PIN);

    gpio_mode_set(BSP_SPI_FLASH_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_SPI_FLASH_MOSI_PIN);
    gpio_output_options_set(BSP_SPI_FLASH_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_SPI_FLASH_MOSI_PIN);

    gpio_mode_set(BSP_SPI_FLASH_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, BSP_SPI_FLASH_CS_PIN);
    gpio_output_options_set(BSP_SPI_FLASH_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_SPI_FLASH_CS_PIN);
    bsp_spi_flash_cs_high();

    spi_i2s_deinit(BSP_SPI_FLASH_PERIPH);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_8;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(BSP_SPI_FLASH_PERIPH, &spi_init_struct);
    spi_enable(BSP_SPI_FLASH_PERIPH);
}

uint32_t bsp_spi_flash_read_id(void)
{
    uint32_t id;

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_READ_ID);
    id = ((uint32_t)bsp_spi_flash_transfer(0xFFU) << 16U);
    id |= ((uint32_t)bsp_spi_flash_transfer(0xFFU) << 8U);
    id |= (uint32_t)bsp_spi_flash_transfer(0xFFU);
    bsp_spi_flash_cs_high();

    return id;
}

uint8_t bsp_spi_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    uint32_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    if (bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT) == 0U) {
        return 0U;
    }

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_READ_DATA);
    bsp_spi_flash_send_address(address);

    for (index = 0U; index < length; index++) {
        data[index] = bsp_spi_flash_transfer(0xFFU);
    }

    bsp_spi_flash_cs_high();
    return 1U;
}

uint8_t bsp_spi_flash_page_program(uint32_t address, const uint8_t *data, uint16_t length)
{
    uint16_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    if ((length == 0U) || (length > BSP_SPI_FLASH_PAGE_SIZE)) {
        return 0U;
    }

    if (((address & (BSP_SPI_FLASH_PAGE_SIZE - 1U)) + length) > BSP_SPI_FLASH_PAGE_SIZE) {
        return 0U;
    }

    if (bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT) == 0U) {
        return 0U;
    }

    bsp_spi_flash_write_enable();

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_PAGE_PROGRAM);
    bsp_spi_flash_send_address(address);

    for (index = 0U; index < length; index++) {
        (void)bsp_spi_flash_transfer(data[index]);
    }

    bsp_spi_flash_cs_high();
    return bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT);
}

uint8_t bsp_spi_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t offset;
    uint16_t chunk;
    uint16_t page_remain;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    offset = 0U;
    while (offset < length) {
        page_remain = (uint16_t)(BSP_SPI_FLASH_PAGE_SIZE - ((address + offset) & (BSP_SPI_FLASH_PAGE_SIZE - 1U)));
        chunk = (uint16_t)(length - offset);
        if (chunk > page_remain) {
            chunk = page_remain;
        }

        if (bsp_spi_flash_page_program(address + offset, &data[offset], chunk) == 0U) {
            return 0U;
        }

        offset += chunk;
    }

    return 1U;
}

uint8_t bsp_spi_flash_sector_erase(uint32_t address)
{
    if (bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT) == 0U) {
        return 0U;
    }

    bsp_spi_flash_write_enable();

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_SECTOR_ERASE);
    bsp_spi_flash_send_address(address);
    bsp_spi_flash_cs_high();

    return bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT);
}

uint8_t bsp_spi_flash_chip_erase(void)
{
    if (bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT) == 0U) {
        return 0U;
    }

    bsp_spi_flash_write_enable();

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_CHIP_ERASE);
    bsp_spi_flash_cs_high();

    return bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT * 20U);
}
