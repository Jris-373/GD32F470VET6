#include "protocol_ascii_hex.h"

char protocol_hex_nibble_to_char(uint8_t value)
{
    value &= 0x0FU;
    if (value < 10U) {
        return (char)('0' + value);
    }

    return (char)('A' + value - 10U);
}

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

protocol_hex_status_t protocol_hex_encode(const uint8_t *bytes, uint16_t byte_len, char *ascii, uint16_t ascii_size, uint16_t *ascii_len)
{
    uint16_t index;
    uint16_t required;

    if (((bytes == 0) && (byte_len != 0U)) || (ascii == 0)) {
        return PROTOCOL_HEX_ERR_PARAM;
    }

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

protocol_hex_status_t protocol_hex_decode(const char *ascii, uint16_t ascii_len, uint8_t *bytes, uint16_t byte_size, uint16_t *byte_len)
{
    uint16_t index;
    uint16_t required;
    uint8_t high;
    uint8_t low;

    if (((ascii == 0) && (ascii_len != 0U)) || (bytes == 0)) {
        return PROTOCOL_HEX_ERR_PARAM;
    }

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
