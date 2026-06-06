#include "bsp_oled.h"
#include "bsp_i2c.h"
#include "oled_font.h"
#include "systick.h"

#define BSP_OLED_CMD         0U
#define BSP_OLED_DATA        1U
#define BSP_OLED_I2C_CONTROL_CMD  0x00U
#define BSP_OLED_I2C_CONTROL_DATA 0x40U

static uint8_t s_oled_gram[BSP_OLED_WIDTH][BSP_OLED_PAGE_COUNT];

static uint32_t bsp_oled_pow(uint8_t base, uint8_t exp)
{
    uint32_t result = 1U;

    while (exp != 0U) {
        result *= base;
        exp--;
    }

    return result;
}

static void bsp_oled_write_byte(uint8_t data, uint8_t type)
{
    uint8_t packet[2];

    packet[0] = (type == BSP_OLED_CMD) ? BSP_OLED_I2C_CONTROL_CMD : BSP_OLED_I2C_CONTROL_DATA;
    packet[1] = data;
    (void)bsp_i2c0_write(BSP_OLED_I2C_ADDR_7BIT, packet, sizeof(packet), 0U);
}

static void bsp_oled_write_data_block(const uint8_t *data, uint16_t length)
{
    uint8_t packet[17];
    uint16_t offset;
    uint16_t chunk;
    uint16_t index;

    offset = 0U;
    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > 16U) {
            chunk = 16U;
        }

        packet[0] = BSP_OLED_I2C_CONTROL_DATA;
        for (index = 0U; index < chunk; index++) {
            packet[index + 1U] = data[offset + index];
        }

        (void)bsp_i2c0_write(BSP_OLED_I2C_ADDR_7BIT, packet, (uint16_t)(chunk + 1U), 0U);
        offset = (uint16_t)(offset + chunk);
    }
}

static void bsp_oled_config_reg(void)
{
    bsp_oled_write_byte(0xAEU, BSP_OLED_CMD);
    bsp_oled_write_byte(0xD5U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x50U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xA8U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x1FU, BSP_OLED_CMD);
    bsp_oled_write_byte(0xD3U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x00U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x40U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x8DU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x14U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x20U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x02U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xA1U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xC0U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xDAU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x02U, BSP_OLED_CMD);
    bsp_oled_write_byte(0x81U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xEFU, BSP_OLED_CMD);
    bsp_oled_write_byte(0xD9U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xF1U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xDBU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x30U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xA4U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xA6U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xAFU, BSP_OLED_CMD);
}

void bsp_oled_init(void)
{
    bsp_i2c0_init();
    delay_1ms(20U);
    bsp_oled_config_reg();
    bsp_oled_clear();
}

void bsp_oled_display_on(void)
{
    bsp_oled_write_byte(0x8DU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x14U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xAFU, BSP_OLED_CMD);
}

void bsp_oled_display_off(void)
{
    bsp_oled_write_byte(0x8DU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x10U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xAEU, BSP_OLED_CMD);
}

void bsp_oled_refresh(void)
{
    uint8_t page;
    uint8_t column;
    uint8_t line[BSP_OLED_WIDTH];

    for (page = 0U; page < BSP_OLED_PAGE_COUNT; page++) {
        bsp_oled_write_byte((uint8_t)(0xB0U + page), BSP_OLED_CMD);
        bsp_oled_write_byte(0x00U, BSP_OLED_CMD);
        bsp_oled_write_byte(0x10U, BSP_OLED_CMD);

        for (column = 0U; column < BSP_OLED_WIDTH; column++) {
            line[column] = s_oled_gram[column][page];
        }

        bsp_oled_write_data_block(line, BSP_OLED_WIDTH);
    }
}

void bsp_oled_clear(void)
{
    bsp_oled_fill(0x00U);
}

void bsp_oled_fill(uint8_t data)
{
    uint8_t page;
    uint8_t column;

    for (page = 0U; page < BSP_OLED_PAGE_COUNT; page++) {
        for (column = 0U; column < BSP_OLED_WIDTH; column++) {
            s_oled_gram[column][page] = data;
        }
    }

    bsp_oled_refresh();
}

void bsp_oled_draw_point(uint8_t x, uint8_t y, uint8_t on)
{
    uint8_t page;
    uint8_t bit;
    uint8_t mask;

    if ((x >= BSP_OLED_WIDTH) || (y >= BSP_OLED_HEIGHT)) {
        return;
    }

    page = (uint8_t)(BSP_OLED_PAGE_COUNT - 1U - (y / 8U));
    bit  = (uint8_t)(y % 8U);
    mask = (uint8_t)(1U << (7U - bit));

    if (on != 0U) {
        s_oled_gram[x][page] |= mask;
    } else {
        s_oled_gram[x][page] &= (uint8_t)(~mask);
    }
}

void bsp_oled_show_char(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t mode)
{
    uint8_t temp;
    uint8_t t1;
    uint8_t t2;
    uint8_t y0;
    uint8_t font_index;

    if ((ch < ' ') || (ch > '~')) {
        ch = '?';
    }

    if ((size != 12U) && (size != 16U)) {
        size = 16U;
    }

    y0 = y;
    font_index = (uint8_t)(ch - ' ');

    for (t1 = 0U; t1 < size; t1++) {
        if (size == 12U) {
            temp = g_iASCII1206[font_index][t1];
        } else {
            temp = g_iASCII1608[font_index][t1];
        }

        for (t2 = 0U; t2 < 8U; t2++) {
            if ((temp & 0x80U) != 0U) {
                bsp_oled_draw_point(x, y, mode);
            } else {
                bsp_oled_draw_point(x, y, (uint8_t)!mode);
            }

            temp <<= 1U;
            y++;

            if ((uint8_t)(y - y0) == size) {
                y = y0;
                x++;
                break;
            }
        }
    }
}

void bsp_oled_show_string(uint8_t x, uint8_t y, const char *str)
{
    while ((str != 0) && (*str != '\0')) {
        if (x > (BSP_OLED_WIDTH - 8U)) {
            x = 0U;
            y = (uint8_t)(y + 16U);
        }

        if (y > (BSP_OLED_HEIGHT - 16U)) {
            x = 0U;
            y = 0U;
            bsp_oled_clear();
        }

        bsp_oled_show_char(x, y, *str, 16U, 1U);
        x = (uint8_t)(x + 8U);
        str++;
    }

    bsp_oled_refresh();
}

void bsp_oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t index;
    uint8_t digit;
    uint8_t has_value;

    has_value = 0U;
    for (index = 0U; index < len; index++) {
        digit = (uint8_t)((num / bsp_oled_pow(10U, (uint8_t)(len - index - 1U))) % 10U);
        if ((has_value == 0U) && (index < (uint8_t)(len - 1U)) && (digit == 0U)) {
            bsp_oled_show_char((uint8_t)(x + (size / 2U) * index), y, ' ', size, 1U);
            continue;
        }

        has_value = 1U;
        bsp_oled_show_char((uint8_t)(x + (size / 2U) * index), y, (char)(digit + '0'), size, 1U);
    }

    bsp_oled_refresh();
}
