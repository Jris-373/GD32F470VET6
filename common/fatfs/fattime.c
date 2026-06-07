#include "diskio.h"
#include "bsp_rtc.h"

DWORD get_fattime(void)
{
    bsp_rtc_datetime_t datetime;
    uint16_t year;

    bsp_rtc_get_datetime(&datetime);
    year = datetime.year;
    if (year < 1980U) {
        year = 1980U;
    }

    return ((DWORD)(year - 1980U) << 25U) |
           ((DWORD)datetime.month << 21U) |
           ((DWORD)datetime.date << 16U) |
           ((DWORD)datetime.hour << 11U) |
           ((DWORD)datetime.minute << 5U) |
           ((DWORD)(datetime.second / 2U));
}
