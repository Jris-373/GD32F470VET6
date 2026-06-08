#include "bsp_oled.h"
#include "bsp_i2c.h"
#include "oled_font.h"
#include "systick.h"

#define BSP_OLED_CMD         0U
#define BSP_OLED_DATA        1U
#define BSP_OLED_I2C_CONTROL_CMD  0x00U
#define BSP_OLED_I2C_CONTROL_DATA 0x40U

static uint8_t s_oled_gram[BSP_OLED_WIDTH][BSP_OLED_PAGE_COUNT];

/*
 * 计算整数幂，用于数字显示时提取各位。
 * 参数 base 为底数，exp 为指数；返回 base 的 exp 次方。
 */
static uint32_t bsp_oled_pow(uint8_t base, uint8_t exp)
{
    uint32_t result = 1U;

    while (exp != 0U) {
        result *= base;
        exp--;
    }

    return result;
}

/*
 * 通过 I2C 向 OLED 写入一个命令或数据字节。
 * 参数 data 为待写字节，type 指定命令/数据；无返回值，底层 I2C 错误在此处忽略。
 */
static void bsp_oled_write_byte(uint8_t data, uint8_t type)
{
    uint8_t packet[2];

    packet[0] = (type == BSP_OLED_CMD) ? BSP_OLED_I2C_CONTROL_CMD : BSP_OLED_I2C_CONTROL_DATA;
    packet[1] = data;
    (void)bsp_i2c0_write(BSP_OLED_I2C_ADDR_7BIT, packet, sizeof(packet), 0U);
}

/*
 * 向 OLED 连续写入显示数据块。
 * 参数 data/length 为待刷新的 GRAM 数据；无返回值。函数按 16 字节分包，避免临时缓冲区过大。
 */
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

/*
 * 配置 OLED 控制寄存器。
 * 无传入参数，无返回值；初始化显示方向、扫描方式、对比度、电荷泵等 SSD1306 参数。
 */
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

/*
 * 初始化 OLED 显示模块。
 * 无传入参数，无返回值；函数会初始化 I2C0、等待 OLED 上电稳定、写入配置并清屏。
 */
void bsp_oled_init(void)
{
    bsp_i2c0_init();
    delay_1ms(20U);
    bsp_oled_config_reg();
    bsp_oled_clear();
}

/*
 * 打开 OLED 显示。
 * 无传入参数，无返回值；低功耗唤醒后可调用该函数恢复显示。
 */
void bsp_oled_display_on(void)
{
    bsp_oled_write_byte(0x8DU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x14U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xAFU, BSP_OLED_CMD);
}

/*
 * 关闭 OLED 显示。
 * 无传入参数，无返回值；进入睡眠前调用可降低系统功耗。
 */
void bsp_oled_display_off(void)
{
    bsp_oled_write_byte(0x8DU, BSP_OLED_CMD);
    bsp_oled_write_byte(0x10U, BSP_OLED_CMD);
    bsp_oled_write_byte(0xAEU, BSP_OLED_CMD);
}

/*
 * 将软件 GRAM 刷新到 OLED 屏幕。
 * 无传入参数，无返回值；按页写入 128 列数据。
 */
void bsp_oled_refresh(void)
{
    uint8_t page;
    uint8_t column;
    uint8_t line[BSP_OLED_WIDTH];

    /* OLED 按 page 寻址，每页 8 像素高，逐页刷完整屏。 */
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

/*
 * 清空 OLED 显示。
 * 无传入参数，无返回值；内部用 0x00 填充整个显存并刷新。
 */
void bsp_oled_clear(void)
{
    bsp_oled_fill(0x00U);
}

/*
 * 用指定字节填充 OLED 显存。
 * 参数 data 为填充值；无返回值，常用于清屏或全屏点亮测试。
 */
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

/*
 * 在软件 GRAM 中绘制单个像素点。
 * 参数 x/y 为像素坐标，on 非 0 表示点亮，0 表示熄灭；坐标越界时直接返回。
 */
void bsp_oled_draw_point(uint8_t x, uint8_t y, uint8_t on)
{
    uint8_t page;
    uint8_t bit;
    uint8_t mask;

    if ((x >= BSP_OLED_WIDTH) || (y >= BSP_OLED_HEIGHT)) {
        return;
    }

    /* 当前 OLED 模块安装方向与常规页序相反，因此 page 需要反向换算。 */
    page = (uint8_t)(BSP_OLED_PAGE_COUNT - 1U - (y / 8U));
    bit  = (uint8_t)(y % 8U);
    mask = (uint8_t)(1U << (7U - bit));

    if (on != 0U) {
        s_oled_gram[x][page] |= mask;
    } else {
        s_oled_gram[x][page] &= (uint8_t)(~mask);
    }
}

/*
 * 显示一个 ASCII 字符。
 * 参数 x/y 为左上角坐标，ch 为字符，size 支持 12 或 16，mode 控制正常/反色绘制。
 */
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

/*
 * 显示以 '\0' 结尾的字符串。
 * 参数 x/y 为起始坐标，str 为字符串指针；无返回值，超出屏幕时自动换行或清屏回到起点。
 */
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

/*
 * 显示固定宽度的无符号整数。
 * 参数 x/y 为起始坐标，num 为数值，len 为显示位数，size 为字体高度；无返回值。
 */
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
