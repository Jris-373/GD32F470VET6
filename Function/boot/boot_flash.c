#include "boot_flash.h"
#include "boot_crc32.h"
#include "gd32f4xx_fmc.h"

/* 检查内部 Flash 写入范围是否只落在参数区和 App 区。
 * 参数：address 为起始地址；length 为字节数。
 * 返回：1 表示范围有效，0 表示越界或地址回绕。
 */
static uint8_t boot_flash_range_is_valid(uint32_t address, uint32_t length)
{
    uint32_t end;

    if (length == 0U) {
        return 1U;
    }

    end = address + length;
    if (end < address) {
        return 0U;
    }

    if (address < BOOT_PARAM_ADDR) {
        return 0U;
    }

    if (end > BOOT_INTERNAL_FLASH_END_ADDR) {
        return 0U;
    }

    return 1U;
}

/* 擦除内部 Flash 指定范围。
 * 参数：address 为起始地址；length 为需要擦除的字节数。
 * 返回：1 表示擦除成功，0 表示范围非法或 FMC 擦除失败。
 */
uint8_t boot_flash_erase(uint32_t address, uint32_t length)
{
    uint32_t page_addr;
    uint32_t end;

    if (boot_flash_range_is_valid(address, length) == 0U) {
        return 0U;
    }

    if (length == 0U) {
        return 1U;
    }

    page_addr = address & ~(BOOT_INTERNAL_FLASH_PAGE_SIZE - 1U);
    end = address + length;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);

    while (page_addr < end) {
        /* GD32F470 这里按内部 Flash 页大小逐页擦除。 */
        if (fmc_page_erase(page_addr) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        page_addr += BOOT_INTERNAL_FLASH_PAGE_SIZE;
    }

    fmc_lock();
    return 1U;
}

/* 向内部 Flash 写入数据。
 * 参数：address 为目标地址；data 为待写数据；length 为字节数。
 * 返回：1 表示写入成功，0 表示范围非法、参数错误或 FMC 写入失败。
 */
uint8_t boot_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t index;

    if ((data == 0) && (length != 0U)) {
        return 0U;
    }

    if (boot_flash_range_is_valid(address, length) == 0U) {
        return 0U;
    }

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);

    /* 使用字节编程，牺牲速度换取对任意长度 App 镜像的兼容性。 */
    for (index = 0U; index < length; index++) {
        if (fmc_byte_program(address + index, data[index]) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
    }

    fmc_lock();
    return 1U;
}

/* 计算内部 Flash 指定范围的 CRC32。
 * 参数：address 为起始地址；length 为字节数。
 * 返回：CRC32 校验值。
 */
uint32_t boot_flash_crc32(uint32_t address, uint32_t length)
{
    return boot_crc32_calc((const uint8_t *)address, length);
}
