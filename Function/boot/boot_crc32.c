#include "boot_crc32.h"

/* 初始化 CRC32 计算上下文。
 * 参数：无。
 * 返回：CRC32 初始值。
 */
uint32_t boot_crc32_start(void)
{
    return 0xFFFFFFFFU;
}

/* 增量计算 CRC32，支持分块读取固件时连续更新。
 * 参数：crc 为上一次 CRC 状态；data 为数据缓冲区；length 为字节数。
 * 返回：更新后的 CRC 状态，最后还需要调用 boot_crc32_finish。
 */
uint32_t boot_crc32_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    uint32_t byte_index;
    uint8_t bit_index;

    if ((data == 0) && (length != 0U)) {
        return crc;
    }

    for (byte_index = 0U; byte_index < length; byte_index++) {
        /* 使用以太网/ZIP 常见的反射 CRC32 多项式 0xEDB88320。 */
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

/* 结束 CRC32 计算。
 * 参数：crc 为 boot_crc32_update 返回的中间值。
 * 返回：最终 CRC32 值。
 */
uint32_t boot_crc32_finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}

/* 一次性计算一段内存的 CRC32。
 * 参数：data 为数据缓冲区；length 为字节数。
 * 返回：最终 CRC32 值。
 */
uint32_t boot_crc32_calc(const uint8_t *data, uint32_t length)
{
    return boot_crc32_finish(boot_crc32_update(boot_crc32_start(), data, length));
}
