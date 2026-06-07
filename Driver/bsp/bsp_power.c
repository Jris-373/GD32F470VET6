#include "bsp_power.h"
#include "bsp_rtc.h"
#include "systick.h"
#include "system_gd32f4xx.h"

static uint8_t bsp_power_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static void bsp_power_soft_delay(uint32_t time)
{
    volatile uint32_t index;

    for (index = 0U; index < (time * 10U); index++) {
    }
}

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

void bsp_power_sleep(void)
{
    pmu_to_sleepmode(WFI_CMD);
}

uint8_t bsp_power_deepsleep_rtc_alarm(uint8_t seconds)
{
    if (bsp_power_config_rtc_alarm(seconds) == 0U) {
        return 0U;
    }

    rcu_periph_clock_enable(RCU_PMU);
    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    systick_suspend();
    bsp_power_switch_to_irc16m_before_deepsleep();
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

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
