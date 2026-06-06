#ifndef BSP_OLED_H
#define BSP_OLED_H

#include "gd32f4xx.h"

#define BSP_OLED_WIDTH         128U
#define BSP_OLED_HEIGHT        32U
#define BSP_OLED_PAGE_COUNT    4U
#define BSP_OLED_I2C_ADDR_7BIT 0x3CU

void bsp_oled_init(void);
void bsp_oled_display_on(void);
void bsp_oled_display_off(void);
void bsp_oled_refresh(void);
void bsp_oled_clear(void);
void bsp_oled_fill(uint8_t data);
void bsp_oled_draw_point(uint8_t x, uint8_t y, uint8_t on);
void bsp_oled_show_char(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t mode);
void bsp_oled_show_string(uint8_t x, uint8_t y, const char *str);
void bsp_oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

#endif /* BSP_OLED_H */
