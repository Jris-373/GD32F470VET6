#ifndef PROTOCOL_ASCII_HEX_H
#define PROTOCOL_ASCII_HEX_H

#include <stdint.h>

typedef enum {
    PROTOCOL_HEX_OK = 0,
    PROTOCOL_HEX_ERR_PARAM,
    PROTOCOL_HEX_ERR_CHAR,
    PROTOCOL_HEX_ERR_SIZE,
} protocol_hex_status_t;

char protocol_hex_nibble_to_char(uint8_t value);
uint8_t protocol_hex_char_to_nibble(char ch, uint8_t *value);
protocol_hex_status_t protocol_hex_encode(const uint8_t *bytes, uint16_t byte_len, char *ascii, uint16_t ascii_size, uint16_t *ascii_len);
protocol_hex_status_t protocol_hex_decode(const char *ascii, uint16_t ascii_len, uint8_t *bytes, uint16_t byte_size, uint16_t *byte_len);

#endif /* PROTOCOL_ASCII_HEX_H */
