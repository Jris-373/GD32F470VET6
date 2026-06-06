#ifndef PROJECT_NVIC_H
#define PROJECT_NVIC_H

#include <stdint.h>

typedef enum {
    NVIC_FAULT_NONE = 0,
    NVIC_FAULT_NMI,
    NVIC_FAULT_HARDFAULT,
    NVIC_FAULT_MEMMANAGE,
    NVIC_FAULT_BUSFAULT,
    NVIC_FAULT_USAGEFAULT,
    NVIC_FAULT_SVC,
    NVIC_FAULT_DEBUGMON,
    NVIC_FAULT_PENDSV
} nvic_fault_id_t;

extern volatile uint32_t g_nvic_fault_id;
extern volatile uint32_t g_nvic_fault_cfsr;
extern volatile uint32_t g_nvic_fault_hfsr;
extern volatile uint32_t g_nvic_fault_dfsr;
extern volatile uint32_t g_nvic_fault_afsr;
extern volatile uint32_t g_nvic_fault_mmfar;
extern volatile uint32_t g_nvic_fault_bfar;
extern volatile uint32_t g_nvic_fault_shcsr;
extern volatile uint32_t g_nvic_fault_icsr;
extern volatile uint32_t g_nvic_fault_vtor;
extern volatile uint32_t g_nvic_fault_ispr0;
extern volatile uint32_t g_nvic_fault_ispr1;
extern volatile uint32_t g_nvic_fault_ispr2;
extern volatile uint32_t g_nvic_fault_iabr0;
extern volatile uint32_t g_nvic_fault_iabr1;
extern volatile uint32_t g_nvic_fault_iabr2;
extern volatile uint32_t g_nvic_fault_stack_r0;
extern volatile uint32_t g_nvic_fault_stack_r1;
extern volatile uint32_t g_nvic_fault_stack_r2;
extern volatile uint32_t g_nvic_fault_stack_r3;
extern volatile uint32_t g_nvic_fault_stack_r12;
extern volatile uint32_t g_nvic_fault_stack_lr;
extern volatile uint32_t g_nvic_fault_stack_pc;
extern volatile uint32_t g_nvic_fault_stack_xpsr;

void nvic_nmi_callback(void);
void nvic_hardfault_callback(void);
void nvic_hardfault_callback_with_stack(uint32_t *stack_frame);
void nvic_memmanage_callback(void);
void nvic_busfault_callback(void);
void nvic_usagefault_callback(void);
void nvic_svc_callback(void);
void nvic_debugmon_callback(void);
void nvic_pendsv_callback(void);
void nvic_systick_callback(void);
void nvic_exti1_callback(void);
void nvic_timer6_callback(void);
void nvic_usart0_callback(void);
void nvic_usart1_callback(void);
void nvic_rtc_alarm_callback(void);

#endif /* PROJECT_NVIC_H */
