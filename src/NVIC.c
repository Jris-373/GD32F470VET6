#include "NVIC.h"
#include "bsp_key.h"
#include "bsp_timer.h"
#include "bsp_uart.h"
#include "gd32f4xx.h"
#include "systick.h"

volatile uint32_t g_nvic_fault_id = NVIC_FAULT_NONE;
volatile uint32_t g_nvic_fault_cfsr = 0U;
volatile uint32_t g_nvic_fault_hfsr = 0U;
volatile uint32_t g_nvic_fault_dfsr = 0U;
volatile uint32_t g_nvic_fault_afsr = 0U;
volatile uint32_t g_nvic_fault_mmfar = 0U;
volatile uint32_t g_nvic_fault_bfar = 0U;
volatile uint32_t g_nvic_fault_shcsr = 0U;
volatile uint32_t g_nvic_fault_icsr = 0U;
volatile uint32_t g_nvic_fault_vtor = 0U;
volatile uint32_t g_nvic_fault_ispr0 = 0U;
volatile uint32_t g_nvic_fault_ispr1 = 0U;
volatile uint32_t g_nvic_fault_ispr2 = 0U;
volatile uint32_t g_nvic_fault_iabr0 = 0U;
volatile uint32_t g_nvic_fault_iabr1 = 0U;
volatile uint32_t g_nvic_fault_iabr2 = 0U;
volatile uint32_t g_nvic_fault_stack_r0 = 0U;
volatile uint32_t g_nvic_fault_stack_r1 = 0U;
volatile uint32_t g_nvic_fault_stack_r2 = 0U;
volatile uint32_t g_nvic_fault_stack_r3 = 0U;
volatile uint32_t g_nvic_fault_stack_r12 = 0U;
volatile uint32_t g_nvic_fault_stack_lr = 0U;
volatile uint32_t g_nvic_fault_stack_pc = 0U;
volatile uint32_t g_nvic_fault_stack_xpsr = 0U;

/* 捕获 Cortex-M fault 相关寄存器，方便调试器查看异常原因。
 * 参数：fault_id 为本项目定义的异常编号。
 * 返回：无。
 */
static void nvic_capture_fault(nvic_fault_id_t fault_id)
{
    g_nvic_fault_id = (uint32_t)fault_id;
    g_nvic_fault_cfsr = SCB->CFSR;
    g_nvic_fault_hfsr = SCB->HFSR;
    g_nvic_fault_dfsr = SCB->DFSR;
    g_nvic_fault_afsr = SCB->AFSR;
    g_nvic_fault_mmfar = SCB->MMFAR;
    g_nvic_fault_bfar = SCB->BFAR;
    g_nvic_fault_shcsr = SCB->SHCSR;
    g_nvic_fault_icsr = SCB->ICSR;
    g_nvic_fault_vtor = SCB->VTOR;
    g_nvic_fault_ispr0 = NVIC->ISPR[0];
    g_nvic_fault_ispr1 = NVIC->ISPR[1];
    g_nvic_fault_ispr2 = NVIC->ISPR[2];
    g_nvic_fault_iabr0 = NVIC->IABR[0];
    g_nvic_fault_iabr1 = NVIC->IABR[1];
    g_nvic_fault_iabr2 = NVIC->IABR[2];
}

/* 捕获异常入栈现场。
 * 参数：stack_frame 指向异常入口自动压栈的 R0/R1/R2/R3/R12/LR/PC/xPSR。
 * 返回：无。
 */
static void nvic_capture_stack(const uint32_t *stack_frame)
{
    if (stack_frame == 0) {
        return;
    }

    g_nvic_fault_stack_r0 = stack_frame[0];
    g_nvic_fault_stack_r1 = stack_frame[1];
    g_nvic_fault_stack_r2 = stack_frame[2];
    g_nvic_fault_stack_r3 = stack_frame[3];
    g_nvic_fault_stack_r12 = stack_frame[4];
    g_nvic_fault_stack_lr = stack_frame[5];
    g_nvic_fault_stack_pc = stack_frame[6];
    g_nvic_fault_stack_xpsr = stack_frame[7];
}

/* fault 后停机等待调试器查看全局变量。
 * 参数：无。
 * 返回：不返回。
 */
static void nvic_fault_loop(void)
{
    while (1) {
    }
}

/* NMI 中断回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_nmi_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_NMI);
    nvic_fault_loop();
}

/* HardFault 回调，不带栈帧版本。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_hardfault_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_HARDFAULT);
    nvic_fault_loop();
}

/* HardFault 回调，带异常栈帧版本。
 * 参数：stack_frame 为汇编入口传入的异常栈帧。
 * 返回：不返回。
 */
void nvic_hardfault_callback_with_stack(uint32_t *stack_frame)
{
    nvic_capture_fault(NVIC_FAULT_HARDFAULT);
    nvic_capture_stack(stack_frame);
    nvic_fault_loop();
}

/* MemManage fault 回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_memmanage_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_MEMMANAGE);
    nvic_fault_loop();
}

/* BusFault 回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_busfault_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_BUSFAULT);
    nvic_fault_loop();
}

/* UsageFault 回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_usagefault_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_USAGEFAULT);
    nvic_fault_loop();
}

/* SVC 异常回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_svc_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_SVC);
    nvic_fault_loop();
}

/* DebugMonitor 异常回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_debugmon_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_DEBUGMON);
    nvic_fault_loop();
}

/* PendSV 异常回调。
 * 参数：无。
 * 返回：不返回。
 */
void nvic_pendsv_callback(void)
{
    nvic_capture_fault(NVIC_FAULT_PENDSV);
    nvic_fault_loop();
}

/* SysTick 中断回调。
 * 参数：无。
 * 返回：无。
 */
void nvic_systick_callback(void)
{
    delay_decrement();
}

/* EXTI1 中断回调，托管 KEY1 外部中断。
 * 参数：无。
 * 返回：无。
 */
void nvic_exti1_callback(void)
{
    bsp_key_exti_irq_handler();
}

/* TIMER6 中断回调。
 * 参数：无。
 * 返回：无。
 */
void nvic_timer6_callback(void)
{
    bsp_timer_irq_handler();
}

/* USART0 中断回调。
 * 参数：无。
 * 返回：无。
 */
void nvic_usart0_callback(void)
{
    bsp_uart0_irq_handler();
}

/* USART1/RS485 中断回调。
 * 参数：无。
 * 返回：无。
 */
void nvic_usart1_callback(void)
{
    bsp_uart1_irq_handler();
}

/* RTC 闹钟中断回调。
 * 参数：无。
 * 返回：无。
 */
void nvic_rtc_alarm_callback(void)
{
    if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
    }

    exti_interrupt_flag_clear(EXTI_17);
}
