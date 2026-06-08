#include "protocol_ascii_hex.h"

/*
 * 将 4 bit 数值转换为 ASCII 十六进制字符。
 * 参数 value 只使用低 4 位；返回 '0'~'9' 或 'A'~'F'。
 */
char protocol_hex_nibble_to_char(uint8_t value)
{
    value &= 0x0FU;
    if (value < 10U) {
        return (char)('0' + value);
    }

    return (char)('A' + value - 10U);
}

/*
 * 将一个 ASCII 十六进制字符转换为 4 bit 数值。
 * 参数 ch 为待解析字符，value 用于返回 0~15 的数值；返回 1 表示成功，0 表示非法字符或参数为空。
 */
uint8_t protocol_hex_char_to_nibble(char ch, uint8_t *value)
{
    if (value == 0) {
        return 0U;
    }

    if ((ch >= '0') && (ch <= '9')) {
        *value = (uint8_t)(ch - '0');
        return 1U;
    }

    if ((ch >= 'A') && (ch <= 'F')) {
        *value = (uint8_t)(ch - 'A' + 10);
        return 1U;
    }

    if ((ch >= 'a') && (ch <= 'f')) {
        *value = (uint8_t)(ch - 'a' + 10);
        return 1U;
    }

    return 0U;
}

/*
 * 将二进制字节流编码为 ASCII 十六进制字符串。
 * 参数 bytes/byte_len 为输入二进制数据，ascii/ascii_size 为输出缓冲区，ascii_len 可返回实际字符数。
 * 返回 PROTOCOL_HEX_OK 表示成功，其他状态表示参数错误或输出空间不足。
 */
protocol_hex_status_t protocol_hex_encode(const uint8_t *bytes, uint16_t byte_len, char *ascii, uint16_t ascii_size, uint16_t *ascii_len)
{
    uint16_t index;
    uint16_t required;

    if (((bytes == 0) && (byte_len != 0U)) || (ascii == 0)) {
        return PROTOCOL_HEX_ERR_PARAM;
    }

    /* 每个二进制字节会扩展成两个 ASCII 十六进制字符。 */
    required = (uint16_t)(byte_len * 2U);
    if (ascii_size < required) {
        return PROTOCOL_HEX_ERR_SIZE;
    }

    for (index = 0U; index < byte_len; index++) {
        ascii[index * 2U] = protocol_hex_nibble_to_char((uint8_t)(bytes[index] >> 4U));
        ascii[index * 2U + 1U] = protocol_hex_nibble_to_char(bytes[index]);
    }

    if (ascii_len != 0) {
        *ascii_len = required;
    }

    return PROTOCOL_HEX_OK;
}

/*
 * 将 ASCII 十六进制字符串解码为二进制字节流。
 * 参数 ascii/ascii_len 为输入字符串，bytes/byte_size 为输出缓冲区，byte_len 可返回实际字节数。
 * 返回 PROTOCOL_HEX_OK 表示成功，其他状态表示长度错误、非法字符或参数错误。
 */
protocol_hex_status_t protocol_hex_decode(const char *ascii, uint16_t ascii_len, uint8_t *bytes, uint16_t byte_size, uint16_t *byte_len)
{
    uint16_t index;
    uint16_t required;
    uint8_t high;
    uint8_t low;

    if (((ascii == 0) && (ascii_len != 0U)) || (bytes == 0)) {
        return PROTOCOL_HEX_ERR_PARAM;
    }

    /* 十六进制字符串必须两个字符表示一个字节，因此长度必须为偶数。 */
    if ((ascii_len & 1U) != 0U) {
        return PROTOCOL_HEX_ERR_SIZE;
    }

    required = (uint16_t)(ascii_len / 2U);
    if (byte_size < required) {
        return PROTOCOL_HEX_ERR_SIZE;
    }

    for (index = 0U; index < required; index++) {
        if ((protocol_hex_char_to_nibble(ascii[index * 2U], &high) == 0U) ||
            (protocol_hex_char_to_nibble(ascii[index * 2U + 1U], &low) == 0U)) {
            return PROTOCOL_HEX_ERR_CHAR;
        }

        bytes[index] = (uint8_t)((high << 4U) | low);
    }

    if (byte_len != 0) {
        *byte_len = required;
    }

    return PROTOCOL_HEX_OK;
}
