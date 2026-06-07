#include "bsp_timer.h"

static volatile uint8_t s_timer_elapsed;
static volatile uint32_t s_timer_tick;

void bsp_timer_init(uint32_t period_ms)
{
    timer_parameter_struct timer_initpara;
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t period;

    if (period_ms == 0U) {
        period_ms = 1U;
    }

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

void bsp_timer_start(void)
{
    timer_enable(BSP_TIMER_PERIPH);
}

void bsp_timer_stop(void)
{
    timer_disable(BSP_TIMER_PERIPH);
}

void bsp_timer_clear_elapsed(void)
{
    s_timer_elapsed = 0U;
}

uint8_t bsp_timer_elapsed(void)
{
    return s_timer_elapsed;
}

uint32_t bsp_timer_get_tick(void)
{
    return s_timer_tick;
}

void bsp_timer_irq_handler(void)
{
    if (timer_interrupt_flag_get(BSP_TIMER_PERIPH, TIMER_INT_FLAG_UP) == RESET) {
        return;
    }

    timer_interrupt_flag_clear(BSP_TIMER_PERIPH, TIMER_INT_FLAG_UP);
    s_timer_tick++;
    s_timer_elapsed = 1U;
}
