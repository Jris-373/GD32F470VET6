#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include "boot_config.h"

uint8_t boot_flash_erase(uint32_t address, uint32_t length);
uint8_t boot_flash_write(uint32_t address, const uint8_t *data, uint32_t length);
uint32_t boot_flash_crc32(uint32_t address, uint32_t length);

#endif /* BOOT_FLASH_H */
