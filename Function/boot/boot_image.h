#ifndef BOOT_IMAGE_H
#define BOOT_IMAGE_H

#include "boot_config.h"

uint8_t boot_is_valid_app_vector(uint32_t stack_addr, uint32_t entry_addr);
uint32_t boot_image_header_crc32(const boot_image_header_t *header);
uint8_t boot_image_header_is_valid(const boot_image_header_t *header);
boot_status_t boot_external_image_read_header(uint32_t slot_addr, boot_image_header_t *header);
boot_status_t boot_external_image_validate(uint32_t slot_addr, boot_image_header_t *header);
boot_status_t boot_apply_external_image(uint32_t slot_addr, boot_image_header_t *applied_header);
uint8_t boot_app_is_present(void);
void boot_jump_to_app(uint32_t app_addr);

#endif /* BOOT_IMAGE_H */
