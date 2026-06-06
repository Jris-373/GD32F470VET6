#ifndef BSP_UART_H
#define BSP_UART_H

#include "gd32f4xx.h"

#ifndef BSP_UART0_USART
#define BSP_UART0_USART       USART0
#endif

#ifndef BSP_UART0_RCU
#define BSP_UART0_RCU         RCU_USART0
#endif

#ifndef BSP_UART0_TX_RCU
#define BSP_UART0_TX_RCU      RCU_GPIOA
#endif

#ifndef BSP_UART0_TX_PORT
#define BSP_UART0_TX_PORT     GPIOA
#endif

#ifndef BSP_UART0_TX_PIN
#define BSP_UART0_TX_PIN      GPIO_PIN_9
#endif

#ifndef BSP_UART0_RX_RCU
#define BSP_UART0_RX_RCU      RCU_GPIOA
#endif

#ifndef BSP_UART0_RX_PORT
#define BSP_UART0_RX_PORT     GPIOA
#endif

#ifndef BSP_UART0_RX_PIN
#define BSP_UART0_RX_PIN      GPIO_PIN_10
#endif

#ifndef BSP_UART0_GPIO_AF
#define BSP_UART0_GPIO_AF     GPIO_AF_7
#endif

#ifndef BSP_UART0_RX_BUFFER_SIZE
#define BSP_UART0_RX_BUFFER_SIZE 128U
#endif

#ifndef BSP_UART1_USART
#define BSP_UART1_USART       USART1
#endif

#ifndef BSP_UART1_RCU
#define BSP_UART1_RCU         RCU_USART1
#endif

#ifndef BSP_UART1_TX_RCU
#define BSP_UART1_TX_RCU      RCU_GPIOD
#endif

#ifndef BSP_UART1_TX_PORT
#define BSP_UART1_TX_PORT     GPIOD
#endif

#ifndef BSP_UART1_TX_PIN
#define BSP_UART1_TX_PIN      GPIO_PIN_5
#endif

#ifndef BSP_UART1_RX_RCU
#define BSP_UART1_RX_RCU      RCU_GPIOD
#endif

#ifndef BSP_UART1_RX_PORT
#define BSP_UART1_RX_PORT     GPIOD
#endif

#ifndef BSP_UART1_RX_PIN
#define BSP_UART1_RX_PIN      GPIO_PIN_6
#endif

#ifndef BSP_UART1_GPIO_AF
#define BSP_UART1_GPIO_AF     GPIO_AF_7
#endif

#ifndef BSP_UART1_RS485_DE_RCU
#define BSP_UART1_RS485_DE_RCU  RCU_GPIOE
#endif

#ifndef BSP_UART1_RS485_DE_PORT
#define BSP_UART1_RS485_DE_PORT GPIOE
#endif

#ifndef BSP_UART1_RS485_DE_PIN
#define BSP_UART1_RS485_DE_PIN  GPIO_PIN_8
#endif

#ifndef BSP_UART1_RX_BUFFER_SIZE
#define BSP_UART1_RX_BUFFER_SIZE 4096U
#endif

void bsp_uart0_init(uint32_t baudrate);
void bsp_uart0_send_byte(uint8_t data);
void bsp_uart0_send_char(uint8_t ch);
void bsp_uart0_send_string(const char *str);
uint8_t bsp_uart0_byte_available(void);
uint8_t bsp_uart0_read_byte(uint8_t *data);
uint8_t bsp_uart0_read_char(uint8_t *ch);
void bsp_uart0_irq_handler(void);

void bsp_uart1_rs485_init(uint32_t baudrate);
void bsp_uart1_rs485_send_byte(uint8_t data);
void bsp_uart1_rs485_send_buffer(const uint8_t *data, uint16_t length);
void bsp_uart1_rs485_send_string(const char *str);
uint8_t bsp_uart1_rs485_byte_available(void);
uint8_t bsp_uart1_rs485_read_byte(uint8_t *data);
void bsp_uart1_rs485_flush_rx(void);
void bsp_uart1_irq_handler(void);

#endif /* BSP_UART_H */
