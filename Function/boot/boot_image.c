#include "boot_image.h"
#include "boot_crc32.h"
#include "boot_flash.h"
#include "boot_param.h"
#include "bsp_spi_flash.h"
#include "systick.h"

#define BOOT_COPY_BUFFER_SIZE 512U

static uint8_t s_boot_buffer[BOOT_COPY_BUFFER_SIZE];

uint8_t boot_is_valid_app_vector(uint32_t stack_addr, uint32_t entry_addr)
{
    uint32_t entry_aligned;

    entry_aligned = entry_addr & ~1U;

    if ((stack_addr < BOOT_SRAM_START_ADDR) || (stack_addr > BOOT_SRAM_END_ADDR)) {
        return 0U;
    }

    if ((entry_addr & 1U) == 0U) {
        return 0U;
    }

    if ((entry_aligned < BOOT_APP_START_ADDR) || (entry_aligned >= BOOT_APP_END_ADDR)) {
        return 0U;
    }

    return 1U;
}

uint32_t boot_image_header_crc32(const boot_image_header_t *header)
{
    boot_image_header_t temp;

    if (header == 0) {
        return 0U;
    }

    temp = *header;
    temp.header_crc32 = 0U;
    return boot_crc32_calc((const uint8_t *)&temp, sizeof(temp));
}

uint8_t boot_image_header_is_valid(const boot_image_header_t *header)
{
    if (header == 0) {
        return 0U;
    }

    if ((header->magic != BOOT_IMAGE_MAGIC) ||
        (header->tail_magic != BOOT_IMAGE_TAIL_MAGIC) ||
        (header->header_size != sizeof(boot_image_header_t)) ||
        (header->state != BOOT_IMAGE_STATE_READY)) {
        return 0U;
    }

    if ((header->image_size == 0U) || (header->image_size > BOOT_APP_MAX_SIZE)) {
        return 0U;
    }

    if (header->target_addr != BOOT_APP_START_ADDR) {
        return 0U;
    }

    if (boot_is_valid_app_vector(header->stack_addr, header->entry_addr) == 0U) {
        return 0U;
    }

    return header->header_crc32 == boot_image_header_crc32(header);
}

boot_status_t boot_external_image_read_header(uint32_t slot_addr, boot_image_header_t *header)
{
    if (header == 0) {
        return BOOT_ERR_IMAGE_HEADER;
    }

    if (bsp_spi_flash_read(slot_addr, (uint8_t *)header, sizeof(boot_image_header_t)) == 0U) {
        return BOOT_ERR_FLASH_READ;
    }

    if (boot_image_header_is_valid(header) == 0U) {
        return BOOT_ERR_IMAGE_HEADER;
    }

    return BOOT_OK;
}

boot_status_t boot_external_image_validate(uint32_t slot_addr, boot_image_header_t *header)
{
    boot_status_t status;
    boot_image_header_t local_header;
    uint32_t crc;
    uint32_t remaining;
    uint32_t source_addr;
    uint32_t chunk;

    if (header == 0) {
        header = &local_header;
    }

    status = boot_external_image_read_header(slot_addr, header);
    if (status != BOOT_OK) {
        return status;
    }

    crc = boot_crc32_start();
    remaining = header->image_size;
    source_addr = slot_addr + BOOT_IMAGE_HEADER_BYTES;

    while (remaining != 0U) {
        chunk = remaining;
        if (chunk > sizeof(s_boot_buffer)) {
            chunk = sizeof(s_boot_buffer);
        }

        if (bsp_spi_flash_read(source_addr, s_boot_buffer, chunk) == 0U) {
            return BOOT_ERR_FLASH_READ;
        }

        crc = boot_crc32_update(crc, s_boot_buffer, chunk);
        source_addr += chunk;
        remaining -= chunk;
    }

    if (boot_crc32_finish(crc) != header->image_crc32) {
        return BOOT_ERR_IMAGE_CRC;
    }

    return BOOT_OK;
}

boot_status_t boot_apply_external_image(uint32_t slot_addr, boot_image_header_t *applied_header)
{
    boot_image_header_t header;
    boot_status_t status;
    uint32_t remaining;
    uint32_t source_addr;
    uint32_t target_addr;
    uint32_t chunk;
    uint32_t app_crc;

    status = boot_external_image_validate(slot_addr, &header);
    if (status != BOOT_OK) {
        return status;
    }

    __disable_irq();

    if (boot_flash_erase(BOOT_APP_START_ADDR, header.image_size) == 0U) {
        __enable_irq();
        return BOOT_ERR_FLASH_ERASE;
    }

    remaining = header.image_size;
    source_addr = slot_addr + BOOT_IMAGE_HEADER_BYTES;
    target_addr = BOOT_APP_START_ADDR;

    while (remaining != 0U) {
        chunk = remaining;
        if (chunk > sizeof(s_boot_buffer)) {
            chunk = sizeof(s_boot_buffer);
        }

        if (bsp_spi_flash_read(source_addr, s_boot_buffer, chunk) == 0U) {
            __enable_irq();
            return BOOT_ERR_FLASH_READ;
        }

        if (boot_flash_write(target_addr, s_boot_buffer, chunk) == 0U) {
            __enable_irq();
            return BOOT_ERR_FLASH_WRITE;
        }

        source_addr += chunk;
        target_addr += chunk;
        remaining -= chunk;
    }

    __enable_irq();

    app_crc = boot_flash_crc32(BOOT_APP_START_ADDR, header.image_size);
    if (app_crc != header.image_crc32) {
        return BOOT_ERR_IMAGE_CRC;
    }

    if (boot_param_mark_app_installed(&header) == 0U) {
        return BOOT_ERR_PARAM;
    }

    if (applied_header != 0) {
        *applied_header = header;
    }

    return BOOT_OK;
}

uint8_t boot_app_is_present(void)
{
    uint32_t stack_addr;
    uint32_t entry_addr;

    stack_addr = *((const uint32_t *)BOOT_APP_START_ADDR);
    entry_addr = *((const uint32_t *)(BOOT_APP_START_ADDR + 4U));

    return boot_is_valid_app_vector(stack_addr, entry_addr);
}

void boot_jump_to_app(uint32_t app_addr)
{
    uint32_t stack_addr;
    uint32_t entry_addr;
    void (*app_entry)(void);
    uint8_t index;

    stack_addr = *((const uint32_t *)app_addr);
    entry_addr = *((const uint32_t *)(app_addr + 4U));
    app_entry = (void (*)(void))entry_addr;

    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (index = 0U; index < 8U; index++) {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    SCB->VTOR = app_addr;
    __set_MSP(stack_addr);
    __DSB();
    __ISB();
    __enable_irq();
    app_entry();

    while (1) {
    }
}
