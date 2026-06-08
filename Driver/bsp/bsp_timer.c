#include "bsp_timer.h"

static volatile uint8_t s_timer_elapsed;
static volatile uint32_t s_timer_tick;

/*
 * 初始化基础定时器。
 * 参数 period_ms 为中断周期毫秒数，0 会自动修正为 1ms；无返回值。
 * 该定时器用于替代 SysTick 做业务节拍和 OLED 秒计数。
 */
void bsp_timer_init(uint32_t period_ms)
{
    timer_parameter_struct timer_initpara;
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t period;

    if (period_ms == 0U) {
        period_ms = 1U;
    }

    /* APB1 分频不为 1 时，定时器输入时钟为 APB1 时钟的 2 倍。 */
    timer_clock = rcu_clock_freq_get(CK_APB1);
    if ((RCU_CFG0 & RCU_CFG0_APB1PSC) != RCU_APB1_CKAHB_DIV1) {
        timer_clock *= 2U;
    }
    prescaler = (timer_clock / BSP_TIMER_BASE_HZ);
    if (prescaler > 0U) {
        prescaler--;
    }

    period = (period_ms * (BSP_TIMER_BASE_HZ / 1000U));
    if (period == 0U) {
        period = 1U;
    }
    if (period > 0x10000U) {
        period = 0x10000U;
    }
    period--;

    s_timer_elapsed = 0U;
    s_timer_tick = 0U;

    rcu_periph_clock_enable(BSP_TIMER_RCU);
    timer_deinit(BSP_TIMER_PERIPH);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = (uint16_t)prescaler;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = (uint32_t)period;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0U;
    timer_init(BSP_TIMER_PERIPH, &timer_initpara);
    timer_interrupt_flag_clear(BSP_TIMER_PERIPH, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(BSP_TIMER_PERIPH, TIMER_INT_UP);
    nvic_irq_enable(BSP_TIMER_IRQn, 2U, 0U);
}

/*
 * 启动定时器计数。
 * 无传入参数，无返回值；调用后会按初始化周期产生更新中断。
 */
void bsp_timer_start(void)
{
    timer_enable(BSP_TIMER_PERIPH);
}

/*
 * 停止定时器计数。
 * 无传入参数，无返回值；停止后不再产生周期事件。
 */
void bsp_timer_stop(void)
{
    timer_disable(BSP_TIMER_PERIPH);
}

/*
 * 清除一次周期到达标志。
 * 无传入参数，无返回值；主循环处理完周期任务后调用。
 */
void bsp_timer_clear_elapsed(void)
{
    s_timer_elapsed = 0U;
}

/*
 * 查询定时器周期是否已经到达。
 * 无传入参数；返回 1 表示至少发生过一次更新中断，0 表示尚未到达。
 */
uint8_t bsp_timer_elapsed(void)
{
    return s_timer_elapsed;
}

/*
 * 获取定时器累计 tick。
 * 无传入参数；返回从启动后累计的更新中断次数。
 */
uint32_t bsp_timer_get_tick(void)
{
    return s_timer_tick;
}

/*
 * 定时器中断处理入口。
 * 无传入参数，无返回值；由 NVIC 托管层调用，负责清中断标志并更新业务节拍。
 */
void bsp_timer_irq_handler(void)
{
    if (timer_interrupt_flag_get(BSP_TIMER_PERIPH, TIMER_INT_FLAG_UP) == RESET) {
        return;
    }

    timer_interrupt_flag_clear(BSP_TIMER_PERIPH, TIMER_INT_FLAG_UP);
    s_timer_tick++;
    s_timer_elapsed = 1U;
}
