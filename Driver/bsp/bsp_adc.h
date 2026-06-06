#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "gd32f4xx.h"

#ifndef BSP_ADC_RCU
#define BSP_ADC_RCU            RCU_ADC0
#endif

#ifndef BSP_ADC_PERIPH
#define BSP_ADC_PERIPH         ADC0
#endif

#ifndef BSP_ADC_CH0_GPIO_RCU
#define BSP_ADC_CH0_GPIO_RCU   RCU_GPIOC
#endif

#ifndef BSP_ADC_CH0_GPIO_PORT
#define BSP_ADC_CH0_GPIO_PORT  GPIOC
#endif

#ifndef BSP_ADC_CH0_GPIO_PIN
#define BSP_ADC_CH0_GPIO_PIN   GPIO_PIN_0
#endif

#ifndef BSP_ADC_CH0_CHANNEL
#define BSP_ADC_CH0_CHANNEL    ADC_CHANNEL_10
#endif

#ifndef BSP_ADC_CH1_GPIO_RCU
#define BSP_ADC_CH1_GPIO_RCU   RCU_GPIOC
#endif

#ifndef BSP_ADC_CH1_GPIO_PORT
#define BSP_ADC_CH1_GPIO_PORT  GPIOC
#endif

#ifndef BSP_ADC_CH1_GPIO_PIN
#define BSP_ADC_CH1_GPIO_PIN   GPIO_PIN_1
#endif

#ifndef BSP_ADC_CH1_CHANNEL
#define BSP_ADC_CH1_CHANNEL    ADC_CHANNEL_11
#endif

#ifndef BSP_ADC_VREF_MV
#define BSP_ADC_VREF_MV        3300U
#endif

#define BSP_ADC_MAX_RAW        4095U

typedef enum {
    BSP_ADC_INPUT_CH0 = 0,
    BSP_ADC_INPUT_CH1,
} bsp_adc_input_t;

void bsp_adc_init(void);
uint16_t bsp_adc_read_input_raw(bsp_adc_input_t input);
uint8_t bsp_adc_read_input_raw_timeout(bsp_adc_input_t input, uint16_t *raw, uint32_t timeout);
uint16_t bsp_adc_read_raw(void);
uint8_t bsp_adc_read_raw_timeout(uint16_t *raw, uint32_t timeout);
uint16_t bsp_adc_read_mv(void);
uint8_t bsp_adc_read_mv_timeout(uint16_t *millivolt, uint32_t timeout);
uint16_t bsp_adc_raw_to_mv(uint16_t raw);

#endif /* BSP_ADC_H */
