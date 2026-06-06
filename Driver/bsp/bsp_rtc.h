#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "gd32f4xx.h"

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} bsp_rtc_datetime_t;

void bsp_rtc_init(void);
uint8_t bsp_rtc_is_configured(void);
uint8_t bsp_rtc_set_datetime(const bsp_rtc_datetime_t *datetime);
void bsp_rtc_get_datetime(bsp_rtc_datetime_t *datetime);

#endif /* BSP_RTC_H */
