#include "bsp_rtc.h"

#define BSP_RTC_BKP_MARK            0x52435431U
#define BSP_RTC_LXTAL_ASYNC_FACTOR  0x7FU
#define BSP_RTC_LXTAL_SYNC_FACTOR   0x00FFU
#define BSP_RTC_IRC32K_ASYNC_FACTOR 0x63U
#define BSP_RTC_IRC32K_SYNC_FACTOR  0x013FU

static uint16_t s_rtc_async_factor = BSP_RTC_LXTAL_ASYNC_FACTOR;
static uint16_t s_rtc_sync_factor  = BSP_RTC_LXTAL_SYNC_FACTOR;

/*
 * 将十进制 0~99 转换为 BCD。
 * 参数 value 为普通十进制数；返回 RTC 寄存器需要的 BCD 格式。
 */
static uint8_t bsp_rtc_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

/*
 * 将 BCD 格式转换为十进制。
 * 参数 value 为 RTC 读取出的 BCD 数；返回普通十进制数。
 */
static uint8_t bsp_rtc_from_bcd(uint8_t value)
{
    return (uint8_t)(((value >> 4U) * 10U) + (value & 0x0FU));
}

/*
 * 将完整年份转换为 RTC 年份 BCD。
 * 参数 year 可以是 2026 或 26；返回 RTC 使用的两位年份 BCD。
 */
static uint8_t bsp_rtc_year_to_bcd(uint16_t year)
{
    if (year >= 2000U) {
        year -= 2000U;
    }

    return bsp_rtc_to_bcd((uint8_t)(year % 100U));
}

/*
 * 选择 RTC 时钟源。
 * 无传入参数；优先使用外部 LXTAL，失败时退回内部 IRC32K，返回 1 表示选源成功。
 */
static uint8_t bsp_rtc_select_clock_source(void)
{
    rcu_osci_on(RCU_LXTAL);
    if (rcu_osci_stab_wait(RCU_LXTAL) == SUCCESS) {
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        s_rtc_async_factor = BSP_RTC_LXTAL_ASYNC_FACTOR;
        s_rtc_sync_factor  = BSP_RTC_LXTAL_SYNC_FACTOR;
        return 1U;
    }

    rcu_osci_on(RCU_IRC32K);
    if (rcu_osci_stab_wait(RCU_IRC32K) == SUCCESS) {
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
        s_rtc_async_factor = BSP_RTC_IRC32K_ASYNC_FACTOR;
        s_rtc_sync_factor  = BSP_RTC_IRC32K_SYNC_FACTOR;
        return 1U;
    }

    return 0U;
}

/*
 * 初始化 RTC 外设。
 * 无传入参数，无返回值；根据备份寄存器标记判断是否首次配置，并同步 RTC 寄存器。
 */
void bsp_rtc_init(void)
{
    uint32_t rtcsrc_flag;

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    rtcsrc_flag = GET_BITS(RCU_BDCTL, 8U, 9U);

    /* 首次启动或 RTC 时钟源丢失时重置备份域，重新选择可用时钟源。 */
    if ((RTC_BKP0 != BSP_RTC_BKP_MARK) || (rtcsrc_flag == 0U)) {
        rcu_bkp_reset_enable();
        rcu_bkp_reset_disable();
        (void)bsp_rtc_select_clock_source();
    } else if (rtcsrc_flag == GET_BITS(RCU_RTCSRC_IRC32K, 8U, 9U)) {
        s_rtc_async_factor = BSP_RTC_IRC32K_ASYNC_FACTOR;
        s_rtc_sync_factor  = BSP_RTC_IRC32K_SYNC_FACTOR;
    } else {
        s_rtc_async_factor = BSP_RTC_LXTAL_ASYNC_FACTOR;
        s_rtc_sync_factor  = BSP_RTC_LXTAL_SYNC_FACTOR;
    }

    rcu_periph_clock_enable(RCU_RTC);
    (void)rtc_register_sync_wait();
}

/*
 * 判断 RTC 是否已经设置过有效时间。
 * 无传入参数；返回 1 表示备份寄存器有配置标记，0 表示尚未设置。
 */
uint8_t bsp_rtc_is_configured(void)
{
    return (RTC_BKP0 == BSP_RTC_BKP_MARK) ? 1U : 0U;
}

/*
 * 设置 RTC 日期时间。
 * 参数 datetime 为年/月/日/星期/时/分/秒；返回 1 表示设置成功，0 表示参数非法或 RTC 初始化失败。
 */
uint8_t bsp_rtc_set_datetime(const bsp_rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_initpara;

    if (datetime == 0) {
        return 0U;
    }

    if ((datetime->month < 1U) || (datetime->month > 12U) ||
        (datetime->date < 1U) || (datetime->date > 31U) ||
        (datetime->weekday < 1U) || (datetime->weekday > 7U) ||
        (datetime->hour > 23U) || (datetime->minute > 59U) || (datetime->second > 59U)) {
        return 0U;
    }

    rtc_initpara.year           = bsp_rtc_year_to_bcd(datetime->year);
    rtc_initpara.month          = bsp_rtc_to_bcd(datetime->month);
    rtc_initpara.date           = bsp_rtc_to_bcd(datetime->date);
    rtc_initpara.day_of_week    = datetime->weekday;
    rtc_initpara.hour           = bsp_rtc_to_bcd(datetime->hour);
    rtc_initpara.minute         = bsp_rtc_to_bcd(datetime->minute);
    rtc_initpara.second         = bsp_rtc_to_bcd(datetime->second);
    rtc_initpara.factor_asyn    = s_rtc_async_factor;
    rtc_initpara.factor_syn     = s_rtc_sync_factor;
    rtc_initpara.am_pm          = RTC_AM;
    /* 赛题时间按 24 小时制显示和回传，RTC 也明确配置为 24 小时格式。 */
    rtc_initpara.display_format = RTC_24HOUR;

    if (rtc_init(&rtc_initpara) != SUCCESS) {
        return 0U;
    }

    RTC_BKP0 = BSP_RTC_BKP_MARK;
    return 1U;
}

/*
 * 读取当前 RTC 日期时间。
 * 参数 datetime 用于返回时间结构；参数为空时直接返回。
 */
void bsp_rtc_get_datetime(bsp_rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_time;

    if (datetime == 0) {
        return;
    }

    rtc_current_time_get(&rtc_time);

    datetime->year    = (uint16_t)(2000U + bsp_rtc_from_bcd(rtc_time.year));
    datetime->month   = bsp_rtc_from_bcd(rtc_time.month);
    datetime->date    = bsp_rtc_from_bcd(rtc_time.date);
    datetime->weekday = rtc_time.day_of_week;
    datetime->hour    = bsp_rtc_from_bcd(rtc_time.hour);
    datetime->minute  = bsp_rtc_from_bcd(rtc_time.minute);
    datetime->second  = bsp_rtc_from_bcd(rtc_time.second);
}
