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

/* 拉高片选，结束当前 SPI FLASH 事务。
 * 参数：无。
 * 返回：无。
 */
static void bsp_spi_flash_cs_high(void)
{
    gpio_bit_set(BSP_SPI_FLASH_CS_PORT, BSP_SPI_FLASH_CS_PIN);
}

/* SPI 全双工发送 1 字节并同步接收 1 字节。
 * 参数：data 为需要发送到 FLASH 的命令、地址或数据字节。
 * 返回：FLASH 同一时刻返回的 1 字节数据。
 */
static uint8_t bsp_spi_flash_transfer(uint8_t data)
{
    while (spi_i2s_flag_get(BSP_SPI_FLASH_PERIPH, SPI_FLAG_TBE) == RESET) {
    }

    spi_i2s_data_transmit(BSP_SPI_FLASH_PERIPH, data);

    while (spi_i2s_flag_get(BSP_SPI_FLASH_PERIPH, SPI_FLAG_RBNE) == RESET) {
    }

    return (uint8_t)spi_i2s_data_receive(BSP_SPI_FLASH_PERIPH);
}

/* 按 24 位地址格式发送 FLASH 地址。
 * 参数：address 为片内读写或擦除起始地址。
 * 返回：无。
 */
static void bsp_spi_flash_send_address(uint32_t address)
{
    (void)bsp_spi_flash_transfer((uint8_t)(address >> 16U));
    (void)bsp_spi_flash_transfer((uint8_t)(address >> 8U));
    (void)bsp_spi_flash_transfer((uint8_t)address);
}

/* 读取 SPI FLASH 状态寄存器。
 * 参数：无。
 * 返回：状态寄存器原始值，bit0 通常表示忙状态。
 */
static uint8_t bsp_spi_flash_read_status(void)
{
    uint8_t status;

    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_READ_STATUS);
    status = bsp_spi_flash_transfer(0xFFU);
    bsp_spi_flash_cs_high();

    return status;
}

/* 等待 FLASH 内部写入或擦除操作结束。
 * 参数：timeout 为轮询次数上限，用于防止硬件异常时死等。
 * 返回：1 表示 FLASH 就绪，0 表示等待超时。
 */
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

/* 发送写使能命令，后续页编程或擦除前必须调用。
 * 参数：无。
 * 返回：无。
 */
static void bsp_spi_flash_write_enable(void)
{
    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_WRITE_ENABLE);
    bsp_spi_flash_cs_high();
}

/* 初始化外部 SPI FLASH 所使用的 SPI 和片选 GPIO。
 * 参数：无。
 * 返回：无。
 */
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

    /* 采用主机、8 位、模式 0，满足常见 W25Q 系列兼容 FLASH 的 JEDEC 访问时序。 */
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

/* 读取 JEDEC ID，用于确认 SPI FLASH 是否在线。
 * 参数：无。
 * 返回：24 位 ID，通常为 厂商ID + 存储类型 + 容量。
 */
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

/* 从 SPI FLASH 连续读取数据。
 * 参数：address 为起始地址；data 为接收缓冲区；length 为读取字节数。
 * 返回：1 表示读取成功，0 表示参数错误或 FLASH 超时。
 */
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

/* 在同一页内写入最多 256 字节数据。
 * 参数：address 为页内起始地址；data 为待写数据；length 为写入长度。
 * 返回：1 表示写入完成，0 表示参数错误、跨页或等待超时。
 */
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

    /* 页编程命令不能跨页，调用方需要提前按页拆分。 */
    bsp_spi_flash_cs_low();
    (void)bsp_spi_flash_transfer(SPI_FLASH_CMD_PAGE_PROGRAM);
    bsp_spi_flash_send_address(address);

    for (index = 0U; index < length; index++) {
        (void)bsp_spi_flash_transfer(data[index]);
    }

    bsp_spi_flash_cs_high();
    return bsp_spi_flash_wait_ready(BSP_SPI_FLASH_TIMEOUT);
}

/* 将任意长度数据按页拆分后写入 SPI FLASH。
 * 参数：address 为起始地址；data 为待写数据；length 为总字节数。
 * 返回：1 表示全部写入成功，0 表示任一页写入失败。
 */
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
        /* 计算当前地址所在页还剩多少空间，避免页编程跨页。 */
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

/* 擦除 address 所在的 4KB 扇区。
 * 参数：address 为扇区内任意地址。
 * 返回：1 表示擦除完成，0 表示等待超时。
 */
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

/* 擦除整片 SPI FLASH。
 * 参数：无。
 * 返回：1 表示擦除完成，0 表示等待超时。
 */
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
