#include "bsp_uart.h"

static volatile uint8_t s_uart0_rx_buffer[BSP_UART0_RX_BUFFER_SIZE];
static volatile uint16_t s_uart0_rx_head;
static volatile uint16_t s_uart0_rx_tail;
static volatile uint8_t s_uart1_rx_buffer[BSP_UART1_RX_BUFFER_SIZE];
static volatile uint16_t s_uart1_rx_head;
static volatile uint16_t s_uart1_rx_tail;

/* 计算 USART0 环形缓冲区的下一个下标。
 * 参数：index 为当前下标。
 * 返回：递增并按缓冲区长度回绕后的下标。
 */
static uint16_t bsp_uart0_next_index(uint16_t index)
{
    index++;

    if (index >= BSP_UART0_RX_BUFFER_SIZE) {
        index = 0U;
    }

    return index;
}

/* 计算 USART1/RS485 环形缓冲区的下一个下标。
 * 参数：index 为当前下标。
 * 返回：递增并按缓冲区长度回绕后的下标。
 */
static uint16_t bsp_uart1_next_index(uint16_t index)
{
    index++;

    if (index >= BSP_UART1_RX_BUFFER_SIZE) {
        index = 0U;
    }

    return index;
}

/* 将 RS485 收发器切换到接收模式。
 * 参数：无。
 * 返回：无。
 */
static void bsp_uart1_rs485_rx_mode(void)
{
    gpio_bit_reset(BSP_UART1_RS485_DE_PORT, BSP_UART1_RS485_DE_PIN);
}

/* 将 RS485 收发器切换到发送模式。
 * 参数：无。
 * 返回：无。
 */
static void bsp_uart1_rs485_tx_mode(void)
{
    gpio_bit_set(BSP_UART1_RS485_DE_PORT, BSP_UART1_RS485_DE_PIN);
}

/* 初始化 USART0，主要用于 USB-CH340 调试串口。
 * 参数：baudrate 为串口波特率。
 * 返回：无。
 */
void bsp_uart0_init(uint32_t baudrate)
{
    s_uart0_rx_head = 0U;
    s_uart0_rx_tail = 0U;

    rcu_periph_clock_enable(BSP_UART0_TX_RCU);
    rcu_periph_clock_enable(BSP_UART0_RX_RCU);
    rcu_periph_clock_enable(BSP_UART0_RCU);

    gpio_af_set(BSP_UART0_TX_PORT, BSP_UART0_GPIO_AF, BSP_UART0_TX_PIN);
    gpio_af_set(BSP_UART0_RX_PORT, BSP_UART0_GPIO_AF, BSP_UART0_RX_PIN);

    gpio_mode_set(BSP_UART0_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_UART0_TX_PIN);
    gpio_output_options_set(BSP_UART0_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_UART0_TX_PIN);

    gpio_mode_set(BSP_UART0_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_UART0_RX_PIN);
    gpio_output_options_set(BSP_UART0_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_UART0_RX_PIN);

    usart_deinit(BSP_UART0_USART);
    usart_baudrate_set(BSP_UART0_USART, baudrate);
    usart_word_length_set(BSP_UART0_USART, USART_WL_8BIT);
    usart_stop_bit_set(BSP_UART0_USART, USART_STB_1BIT);
    usart_parity_config(BSP_UART0_USART, USART_PM_NONE);
    usart_transmit_config(BSP_UART0_USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(BSP_UART0_USART, USART_RECEIVE_ENABLE);
    usart_interrupt_enable(BSP_UART0_USART, USART_INT_RBNE);
    nvic_irq_enable(USART0_IRQn, 1U, 0U);
    usart_enable(BSP_UART0_USART);
}

/* 通过 USART0 阻塞发送 1 字节。
 * 参数：data 为待发送字节。
 * 返回：无。
 */
void bsp_uart0_send_byte(uint8_t data)
{
    while (usart_flag_get(BSP_UART0_USART, USART_FLAG_TBE) == RESET) {
    }

    usart_data_transmit(BSP_UART0_USART, data);
}

/* 通过 USART0 发送 1 个字符。
 * 参数：ch 为待发送字符。
 * 返回：无。
 */
void bsp_uart0_send_char(uint8_t ch)
{
    bsp_uart0_send_byte(ch);
}

/* 通过 USART0 发送以 0 结尾的字符串。
 * 参数：str 为待发送字符串，传入 0 时不发送。
 * 返回：无。
 */
void bsp_uart0_send_string(const char *str)
{
    while ((str != 0) && (*str != '\0')) {
        bsp_uart0_send_byte((uint8_t)*str);
        str++;
    }
}

/* 判断 USART0 接收环形缓冲区是否有数据。
 * 参数：无。
 * 返回：1 表示有数据可读，0 表示为空。
 */
uint8_t bsp_uart0_byte_available(void)
{
    return (s_uart0_rx_head != s_uart0_rx_tail) ? 1U : 0U;
}

/* 从 USART0 接收缓冲区读取 1 字节。
 * 参数：data 为输出指针。
 * 返回：1 表示读到数据，0 表示参数错误或当前无数据。
 */
uint8_t bsp_uart0_read_byte(uint8_t *data)
{
    if ((data == 0) || (bsp_uart0_byte_available() == 0U)) {
        return 0U;
    }

    *data = s_uart0_rx_buffer[s_uart0_rx_tail];
    s_uart0_rx_tail = bsp_uart0_next_index(s_uart0_rx_tail);
    return 1U;
}

/* 从 USART0 读取 1 个字符。
 * 参数：ch 为输出指针。
 * 返回：1 表示读到字符，0 表示无数据或参数错误。
 */
uint8_t bsp_uart0_read_char(uint8_t *ch)
{
    return bsp_uart0_read_byte(ch);
}

/* USART0 接收中断处理，将收到的字节写入环形缓冲区。
 * 参数：无。
 * 返回：无。
 */
void bsp_uart0_irq_handler(void)
{
    uint16_t next_head;
    uint8_t data;

    if (usart_interrupt_flag_get(BSP_UART0_USART, USART_INT_FLAG_RBNE) == RESET) {
        return;
    }

    data = (uint8_t)usart_data_receive(BSP_UART0_USART);
    next_head = bsp_uart0_next_index(s_uart0_rx_head);

    /* 缓冲区满时丢弃本字节，避免覆盖尚未处理的数据。 */
    if (next_head != s_uart0_rx_tail) {
        s_uart0_rx_buffer[s_uart0_rx_head] = data;
        s_uart0_rx_head = next_head;
    }
}

/* 初始化 USART1 + RS485 半双工接口，作为赛题协议主通信口。
 * 参数：baudrate 为 RS485 通信波特率，默认要求 19200。
 * 返回：无。
 */
void bsp_uart1_rs485_init(uint32_t baudrate)
{
    s_uart1_rx_head = 0U;
    s_uart1_rx_tail = 0U;

    rcu_periph_clock_enable(BSP_UART1_TX_RCU);
    rcu_periph_clock_enable(BSP_UART1_RX_RCU);
    rcu_periph_clock_enable(BSP_UART1_RS485_DE_RCU);
    rcu_periph_clock_enable(BSP_UART1_RCU);

    gpio_af_set(BSP_UART1_TX_PORT, BSP_UART1_GPIO_AF, BSP_UART1_TX_PIN);
    gpio_af_set(BSP_UART1_RX_PORT, BSP_UART1_GPIO_AF, BSP_UART1_RX_PIN);

    gpio_mode_set(BSP_UART1_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_UART1_TX_PIN);
    gpio_output_options_set(BSP_UART1_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_UART1_TX_PIN);

    gpio_mode_set(BSP_UART1_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_UART1_RX_PIN);
    gpio_output_options_set(BSP_UART1_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_UART1_RX_PIN);

    gpio_mode_set(BSP_UART1_RS485_DE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BSP_UART1_RS485_DE_PIN);
    gpio_output_options_set(BSP_UART1_RS485_DE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_UART1_RS485_DE_PIN);
    bsp_uart1_rs485_rx_mode();

    usart_deinit(BSP_UART1_USART);
    usart_baudrate_set(BSP_UART1_USART, baudrate);
    usart_word_length_set(BSP_UART1_USART, USART_WL_8BIT);
    usart_stop_bit_set(BSP_UART1_USART, USART_STB_1BIT);
    usart_parity_config(BSP_UART1_USART, USART_PM_NONE);
    usart_transmit_config(BSP_UART1_USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(BSP_UART1_USART, USART_RECEIVE_ENABLE);
    usart_interrupt_enable(BSP_UART1_USART, USART_INT_RBNE);
    nvic_irq_enable(USART1_IRQn, 1U, 1U);
    usart_enable(BSP_UART1_USART);
}

/* 通过 RS485 阻塞发送 1 字节，发送完成后自动切回接收。
 * 参数：data 为待发送字节。
 * 返回：无。
 */
void bsp_uart1_rs485_send_byte(uint8_t data)
{
    bsp_uart1_rs485_tx_mode();

    while (usart_flag_get(BSP_UART1_USART, USART_FLAG_TBE) == RESET) {
    }

    usart_data_transmit(BSP_UART1_USART, data);

    while (usart_flag_get(BSP_UART1_USART, USART_FLAG_TC) == RESET) {
    }

    bsp_uart1_rs485_rx_mode();
}

/* 通过 RS485 阻塞发送一段二进制数据。
 * 参数：data 为待发送缓冲区；length 为字节数。
 * 返回：无。
 */
void bsp_uart1_rs485_send_buffer(const uint8_t *data, uint16_t length)
{
    uint16_t index;

    if ((data == 0) || (length == 0U)) {
        return;
    }

    bsp_uart1_rs485_tx_mode();

    /* 半双工总线发送期间保持 DE 有效，最后等待 TC 后再释放总线。 */
    for (index = 0U; index < length; index++) {
        while (usart_flag_get(BSP_UART1_USART, USART_FLAG_TBE) == RESET) {
        }

        usart_data_transmit(BSP_UART1_USART, data[index]);
    }

    while (usart_flag_get(BSP_UART1_USART, USART_FLAG_TC) == RESET) {
    }

    bsp_uart1_rs485_rx_mode();
}

/* 通过 RS485 发送以 0 结尾的字符串。
 * 参数：str 为待发送字符串，传入 0 时不发送。
 * 返回：无。
 */
void bsp_uart1_rs485_send_string(const char *str)
{
    uint16_t length;

    if (str == 0) {
        return;
    }

    length = 0U;
    while (str[length] != '\0') {
        length++;
    }

    bsp_uart1_rs485_send_buffer((const uint8_t *)str, length);
}

/* 判断 RS485 接收环形缓冲区是否有数据。
 * 参数：无。
 * 返回：1 表示有数据可读，0 表示为空。
 */
uint8_t bsp_uart1_rs485_byte_available(void)
{
    return (s_uart1_rx_head != s_uart1_rx_tail) ? 1U : 0U;
}

/* 从 RS485 接收缓冲区读取 1 字节。
 * 参数：data 为输出指针。
 * 返回：1 表示读到数据，0 表示参数错误或当前无数据。
 */
uint8_t bsp_uart1_rs485_read_byte(uint8_t *data)
{
    if ((data == 0) || (bsp_uart1_rs485_byte_available() == 0U)) {
        return 0U;
    }

    *data = s_uart1_rx_buffer[s_uart1_rx_tail];
    s_uart1_rx_tail = bsp_uart1_next_index(s_uart1_rx_tail);
    return 1U;
}

/* 丢弃当前 RS485 接收缓冲区内的旧数据。
 * 参数：无。
 * 返回：无。
 */
void bsp_uart1_rs485_flush_rx(void)
{
    s_uart1_rx_tail = s_uart1_rx_head;
}

/* USART1 接收中断处理，将协议字节放入 RS485 环形缓冲区。
 * 参数：无。
 * 返回：无。
 */
void bsp_uart1_irq_handler(void)
{
    uint16_t next_head;
    uint8_t data;

    if (usart_interrupt_flag_get(BSP_UART1_USART, USART_INT_FLAG_RBNE) == RESET) {
        if (usart_interrupt_flag_get(BSP_UART1_USART, USART_INT_FLAG_RBNE_ORERR) == RESET) {
            return;
        }
    }

    data = (uint8_t)usart_data_receive(BSP_UART1_USART);
    next_head = bsp_uart1_next_index(s_uart1_rx_head);

    /* 接收溢出标志同样通过读数据寄存器清除；满缓冲时丢弃新字节。 */
    if (next_head != s_uart1_rx_tail) {
        s_uart1_rx_buffer[s_uart1_rx_head] = data;
        s_uart1_rx_head = next_head;
    }
}
