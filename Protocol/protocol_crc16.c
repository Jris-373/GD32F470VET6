#include "protocol_crc16.h"

/*
 * 计算赛题协议使用的 Modbus CRC16。
 * 参数 data/length 为需要参与校验的字节流；返回 16 位 CRC 值，低位多项式为 0xA001。
 */
uint16_t protocol_crc16_modbus(const uint8_t *data, uint16_t length)
{
    uint16_t crc;
    uint16_t index;
    uint8_t bit;

    crc = 0xFFFFU;

    for (index = 0U; index < length; index++) {
        /* 逐字节异或后按位右移，保持与上位机协议 CRC 算法一致。 */
        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}
