#include "bsp_power.h"
#include "bsp_rtc.h"
#include "systick.h"
#include "system_gd32f4xx.h"

/*
 * 将十进制秒/分/时转换为 BCD。
 * 参数 value 为 0~99 的十进制数；返回 RTC 闹钟寄存器需要的 BCD 格式。
 */
static uint8_t bsp_power_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

/*
 * 简单软件延时。
 * 参数 time 为粗略延时系数；无返回值，用于切换系统时钟时等待电压和时钟稳定。
 */
static void bsp_power_soft_delay(uint32_t time)
{
    volatile uint32_t index;

    for (index = 0U; index < (time * 10U); index++) {
    }
}

/*
 * 进入 Deep Sleep 前将系统时钟切换到 IRC16M。
 * 无传入参数，无返回值；逐级降低 AHB 频率，减少高频切换对内核电压的冲击。
 */
static void bsp_power_switch_to_irc16m_before_deepsleep(void)
{
    rcu_osci_on(RCU_IRC16M);
    (void)rcu_osci_stab_wait(RCU_IRC16M);

    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV2);
    bsp_power_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV4);
    bsp_power_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV8);
    bsp_power_soft_delay(0x50U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV16);
    bsp_power_soft_delay(0x50U);
    rcu_system_clock_source_config(RCU_CKSYSSRC_IRC16M);
    bsp_power_soft_delay(200U);
    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);
}

/*
 * 配置 RTC 闹钟作为 Deep Sleep 唤醒源。
 * 参数 seconds 为从当前秒数开始延后的唤醒秒数；返回 1 表示配置成功，0 表示参数或 RTC 配置失败。
 */
static uint8_t bsp_power_config_rtc_alarm(uint8_t seconds)
{
    bsp_rtc_datetime_t now;
    rtc_alarm_struct alarm;
    uint8_t alarm_second;

    if (seconds == 0U) {
        return 0U;
    }

    bsp_rtc_init();
    bsp_rtc_get_datetime(&now);
    alarm_second = (uint8_t)((now.second + seconds) % 60U);

    if (rtc_alarm_disable(RTC_ALARM0) != SUCCESS) {
        return 0U;
    }

    /* 只比较秒字段，日期/小时/分钟全部屏蔽，满足赛题 10 秒自动唤醒需求。 */
    alarm.alarm_mask      = RTC_ALARM_DATE_MASK | RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
    alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    alarm.alarm_day       = bsp_power_to_bcd(now.date);
    alarm.alarm_hour      = bsp_power_to_bcd(now.hour);
    alarm.alarm_minute    = bsp_power_to_bcd(now.minute);
    alarm.alarm_second    = bsp_power_to_bcd(alarm_second);
    alarm.am_pm           = RTC_AM;

    rtc_alarm_config(RTC_ALARM0, &alarm);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_interrupt_flag_clear(EXTI_17);
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_enable(EXTI_17);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);
    nvic_irq_enable(RTC_Alarm_IRQn, 1U, 0U);

    return 1U;
}

/*
 * 进入普通 Sleep 模式。
 * 无传入参数，无返回值；当前项目主要使用 Deep Sleep，该接口保留给后续低功耗扩展。
 */
void bsp_power_sleep(void)
{
    pmu_to_sleepmode(WFI_CMD);
}

/*
 * 配置 RTC 闹钟并进入 Deep Sleep。
 * 参数 seconds 为唤醒延时秒数；返回 1 表示完成睡眠并已恢复时钟，0 表示闹钟配置失败。
 */
uint8_t bsp_power_deepsleep_rtc_alarm(uint8_t seconds)
{
    if (bsp_power_config_rtc_alarm(seconds) == 0U) {
        return 0U;
    }

    rcu_periph_clock_enable(RCU_PMU);
    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    systick_suspend();
    bsp_power_switch_to_irc16m_before_deepsleep();
    /* 执行 WFI 后 MCU 停在 Deep Sleep，直到 RTC 闹钟中断唤醒。 */
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    /* 唤醒后系统时钟需要重新初始化，否则串口和定时器波特率/节拍会异常。 */
    SystemInit();
    SystemCoreClockUpdate();
    systick_config();
    bsp_rtc_init();
    (void)rtc_alarm_disable(RTC_ALARM0);
    rtc_interrupt_disable(RTC_INT_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_interrupt_flag_clear(EXTI_17);

    return 1U;
}
