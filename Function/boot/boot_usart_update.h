#ifndef BOOT_USART_UPDATE_H
#define BOOT_USART_UPDATE_H

#include "boot_config.h"

void boot_usart_protocol_init(void);
void boot_usart_send_heartbeat(void);
void boot_usart_app_poll(void);
uint8_t boot_usart_update_requested(void);
uint8_t boot_usart_bootloader_upgrade_window(void);

#endif /* BOOT_USART_UPDATE_H */
