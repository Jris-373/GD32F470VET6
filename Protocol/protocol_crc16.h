#ifndef PROTOCOL_CRC16_H
#define PROTOCOL_CRC16_H

#include <stdint.h>

uint16_t protocol_crc16_modbus(const uint8_t *data, uint16_t length);

#endif /* PROTOCOL_CRC16_H */
