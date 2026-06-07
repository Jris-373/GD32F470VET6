#include "boot_crc32.h"

uint32_t boot_crc32_start(void)
{
    return 0xFFFFFFFFU;
}

uint32_t boot_crc32_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    uint32_t byte_index;
    uint8_t bit_index;

    if ((data == 0) && (length != 0U)) {
        return crc;
    }

    for (byte_index = 0U; byte_index < length; byte_index++) {
        crc ^= data[byte_index];
        for (bit_index = 0U; bit_index < 8U; bit_index++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

uint32_t boot_crc32_finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}

uint32_t boot_crc32_calc(const uint8_t *data, uint32_t length)
{
    return boot_crc32_finish(boot_crc32_update(boot_crc32_start(), data, length));
}
