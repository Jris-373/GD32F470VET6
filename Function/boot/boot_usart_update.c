#include "boot_usart_update.h"
#include "boot_crc32.h"
#include "boot_flash.h"
#include "boot_image.h"
#include "boot_param.h"
#include "bsp_uart.h"
#include "gd32f4xx.h"
#include "protocol_ascii_hex.h"
#include "protocol_frame.h"
#include "systick.h"

#define BOOT_USART_CMD_ENTER_BOOT     0x0501U
#define BOOT_USART_CMD_RECEIVE_BIN    0x0502U
#define BOOT_USART_CMD_APPLY_BIN      0x0503U
#define BOOT_USART_CMD_HOST_HEARTBEAT 0xFFFFU
#define BOOT_USART_CMD_DEV_HEARTBEAT  0x8888U

#define BOOT_USART_RAW_MAGIC_BYTES    4U
#define BOOT_USART_RAW_MAX_SIZE       (BOOT_APP_MAX_SIZE + BOOT_USART_RAW_MAGIC_BYTES)
#define BOOT_USART_COPY_BUFFER_SIZE   512U
#define BOOT_USART_RS485_TURNAROUND_MS 5U
#define BOOT_USART_INIT_PROMPT_FIRST_DELAY_MS 100U
#define BOOT_USART_INIT_PROMPT_REPEAT_MS 300U
#define BOOT_USART_INIT_PROMPT_WINDOW_MS 5000U

typedef enum {
    BOOT_USART_FRAME_NONE = 0,
    BOOT_USART_FRAME_READY,
    BOOT_USART_FRAME_ERROR,
} boot_usart_frame_status_t;

static char s_usart_ascii_rx[PROTOCOL_FRAME_MAX_ASCII];
static uint16_t s_usart_ascii_len;
static uint16_t s_usart_expected_ascii_len;
static uint16_t s_usart_device_id;
static uint8_t s_usart_baudrate_code;

#if !defined(CMIS_BUILD_APP)
static uint8_t s_usart_fw_buffer[BOOT_USART_RAW_MAX_SIZE];
static uint32_t s_usart_fw_size;
static uint32_t s_usart_fw_app_size;
static uint32_t s_usart_fw_crc32;
static uint32_t s_usart_fw_stack;
static uint32_t s_usart_fw_entry;
static uint8_t s_usart_fw_ready;
#endif

#if !defined(CMIS_BUILD_APP)
/* 按大端格式读取 32 位整数。
 * 参数：data 指向至少 4 字节缓冲区。
 * 返回：解析出的 32 位值。
 */
static uint32_t boot_usart_read_be32(const uint8_t *data)
{
    return (((uint32_t)data[0]) << 24U) |
           (((uint32_t)data[1]) << 16U) |
           (((uint32_t)data[2]) << 8U) |
           ((uint32_t)data[3]);
}

/* 按小端格式读取 32 位整数。
 * 参数：data 指向至少 4 字节缓冲区。
 * 返回：解析出的 32 位值。
 */
static uint32_t boot_usart_read_le32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           (((uint32_t)data[1]) << 8U) |
           (((uint32_t)data[2]) << 16U) |
           (((uint32_t)data[3]) << 24U);
}
#endif

/* 判断字符是否为合法 ASCII HEX 字符。
 * 参数：ch 为待检查字符。
 * 返回：1 表示合法，0 表示非法。
 */
static uint8_t boot_usart_is_hex_char(char ch)
{
    uint8_t value;

    return protocol_hex_char_to_nibble(ch, &value);
}

/* 检查输入字符是否匹配帧头 A5B6 的指定位置。
 * 参数：offset 为当前帧头偏移；ch 为接收到的字符。
 * 返回：1 表示匹配，0 表示不匹配。
 */
static uint8_t boot_usart_start_char_matches(uint16_t offset, char ch)
{
    switch (offset) {
    case 0U:
        return ((ch == 'A') || (ch == 'a')) ? 1U : 0U;

    case 1U:
        return (ch == '5') ? 1U : 0U;

    case 2U:
        return ((ch == 'B') || (ch == 'b')) ? 1U : 0U;

    case 3U:
        return (ch == '6') ? 1U : 0U;

    default:
        return 1U;
    }
}

/* 重置 Bootloader 串口帧接收状态。
 * 参数：无。
 * 返回：无。
 */
static void boot_usart_frame_receiver_reset(void)
{
    s_usart_ascii_len = 0U;
    s_usart_expected_ascii_len = 0U;
}

/* 根据帧长度字段计算完整 ASCII 帧长度。
 * 参数：无，读取当前接收缓存中的长度字段。
 * 返回：1 表示长度合法，0 表示非法或超出缓冲区。
 */
static uint8_t boot_usart_calc_expected_ascii_len(void)
{
    uint8_t high;
    uint8_t low;
    uint16_t payload_len;

    if (s_usart_ascii_len != 16U) {
        return 1U;
    }

    if ((protocol_hex_char_to_nibble(s_usart_ascii_rx[14], &high) == 0U) ||
        (protocol_hex_char_to_nibble(s_usart_ascii_rx[15], &low) == 0U)) {
        return 0U;
    }

    payload_len = (uint16_t)((high << 4U) | low);
    s_usart_expected_ascii_len = (uint16_t)(26U + payload_len * 2U);
    if (s_usart_expected_ascii_len > PROTOCOL_FRAME_MAX_ASCII) {
        return 0U;
    }

    return 1U;
}

/* 从 RS485 接收缓存中轮询一帧协议报文。
 * 参数：frame 为输出协议帧。
 * 返回：NONE 表示暂未收齐，READY 表示解析成功，ERROR 表示帧错误。
 */
static boot_usart_frame_status_t boot_usart_poll_frame(protocol_frame_t *frame)
{
    uint8_t data;
    protocol_status_t status;

    if (frame == 0) {
        return BOOT_USART_FRAME_ERROR;
    }

    while (bsp_uart1_rs485_read_byte(&data) != 0U) {
        /* 升级阶段只接收 ASCII HEX 协议帧，其他字符会重置接收状态。 */
        if (boot_usart_is_hex_char((char)data) == 0U) {
            boot_usart_frame_receiver_reset();
            continue;
        }

        if (s_usart_ascii_len < 4U) {
            if (boot_usart_start_char_matches(s_usart_ascii_len, (char)data) == 0U) {
                boot_usart_frame_receiver_reset();
                if (boot_usart_start_char_matches(0U, (char)data) != 0U) {
                    s_usart_ascii_rx[s_usart_ascii_len] = (char)data;
                    s_usart_ascii_len++;
                }
                continue;
            }
        }

        if (s_usart_ascii_len >= PROTOCOL_FRAME_MAX_ASCII) {
            boot_usart_frame_receiver_reset();
            return BOOT_USART_FRAME_ERROR;
        }

        s_usart_ascii_rx[s_usart_ascii_len] = (char)data;
        s_usart_ascii_len++;

        if ((s_usart_ascii_len == 16U) && (boot_usart_calc_expected_ascii_len() == 0U)) {
            boot_usart_frame_receiver_reset();
            return BOOT_USART_FRAME_ERROR;
        }

        if ((s_usart_expected_ascii_len != 0U) && (s_usart_ascii_len == s_usart_expected_ascii_len)) {
            status = protocol_frame_parse_ascii(s_usart_ascii_rx, s_usart_ascii_len, frame);
            boot_usart_frame_receiver_reset();
            if (status == PROTOCOL_STATUS_OK) {
                return BOOT_USART_FRAME_READY;
            }

            return BOOT_USART_FRAME_ERROR;
        }
    }

    return BOOT_USART_FRAME_NONE;
}

/* 判断协议帧是否发给本设备或广播地址。
 * 参数：frame 为已解析协议帧。
 * 返回：1 表示本设备需要处理，0 表示忽略。
 */
static uint8_t boot_usart_frame_matches_device(const protocol_frame_t *frame)
{
    if (frame == 0) {
        return 0U;
    }

    return ((frame->device_id == s_usart_device_id) || (frame->device_id == PROTOCOL_DEVICE_BROADCAST)) ? 1U : 0U;
}

/* 构造并通过 RS485 发送 Bootloader 协议帧。
 * 参数：type 为帧类型；command 为命令字；payload/length 为负载数据。
 * 返回：无。
 */
static void boot_usart_send_frame(uint8_t type, uint16_t command, const uint8_t *payload, uint8_t length)
{
    protocol_frame_t frame;
    char ascii[PROTOCOL_FRAME_MAX_ASCII];
    uint16_t ascii_len;
    uint16_t index;

    frame.device_id = s_usart_device_id;
    frame.type = type;
    frame.command = command;
    frame.length = length;

    for (index = 0U; index < length; index++) {
        frame.payload[index] = payload[index];
    }

    if (protocol_frame_build_ascii(&frame, ascii, sizeof(ascii), &ascii_len) == PROTOCOL_STATUS_OK) {
        /* 发送前留出收发器方向切换时间，降低 RS485 首字节丢失概率。 */
        delay_1ms(BOOT_USART_RS485_TURNAROUND_MS);
        bsp_uart1_rs485_send_buffer((const uint8_t *)ascii, ascii_len);
    }
}

/* 发送指定命令的成功应答。
 * 参数：command 为需要应答的命令字。
 * 返回：无。
 */
static void boot_usart_send_ok(uint16_t command)
{
    uint8_t payload;

    payload = 0xFFU;
    boot_usart_send_frame(PROTOCOL_TYPE_RESPONSE, command, &payload, 1U);
}

/* 发送指定命令的错误应答。
 * 参数：command 为需要应答的命令字。
 * 返回：无。
 */
static void boot_usart_send_error(uint16_t command)
{
    boot_usart_send_frame(PROTOCOL_TYPE_ERROR, command, 0, 0U);
}

/* 处理 Bootloader 和 App 都支持的通用帧。
 * 参数：frame 为已解析协议帧。
 * 返回：无。
 */
static void boot_usart_handle_common_frame(const protocol_frame_t *frame)
{
    if (boot_usart_frame_matches_device(frame) == 0U) {
        return;
    }

    if ((frame->type == PROTOCOL_TYPE_HEARTBEAT) && (frame->command == BOOT_USART_CMD_HOST_HEARTBEAT)) {
        boot_usart_send_heartbeat();
    }
}

/* 初始化 Bootloader/App 共用的串口升级协议状态。
 * 参数：无。
 * 返回：无。
 */
void boot_usart_protocol_init(void)
{
    boot_usart_frame_receiver_reset();
    s_usart_device_id = boot_param_get_device_id();
    s_usart_baudrate_code = boot_param_get_baudrate_code();
    bsp_uart1_rs485_init(boot_param_baudrate_from_code(s_usart_baudrate_code));
}

/* 发送设备心跳帧。
 * 参数：无。
 * 返回：无。
 */
void boot_usart_send_heartbeat(void)
{
    boot_usart_send_frame(PROTOCOL_TYPE_HEARTBEAT, BOOT_USART_CMD_DEV_HEARTBEAT, 0, 0U);
}

/* App 侧轮询 0501 进入 Bootloader 命令和心跳。
 * 参数：无。
 * 返回：无；收到合法 0501 后会写参数并复位。
 */
void boot_usart_app_poll(void)
{
    protocol_frame_t frame;
    boot_usart_frame_status_t status;

    status = boot_usart_poll_frame(&frame);
    if (status == BOOT_USART_FRAME_NONE) {
        return;
    }

    if (status == BOOT_USART_FRAME_ERROR) {
        boot_usart_send_error(PROTOCOL_CMD_ERROR);
        return;
    }

    if (boot_usart_frame_matches_device(&frame) == 0U) {
        return;
    }

    if ((frame.type == PROTOCOL_TYPE_COMMAND) &&
        (frame.command == BOOT_USART_CMD_ENTER_BOOT) &&
        (frame.length == 0U)) {
        /* 先持久化串口升级请求，再复位进入 Bootloader。 */
        if (boot_param_mark_usart_request() != 0U) {
            boot_usart_send_ok(BOOT_USART_CMD_ENTER_BOOT);
            delay_1ms(50U);
            NVIC_SystemReset();
        } else {
            boot_usart_send_error(BOOT_USART_CMD_ENTER_BOOT);
        }
        return;
    }

    boot_usart_handle_common_frame(&frame);
}

/* 查询是否存在串口升级请求。
 * 参数：无。
 * 返回：1 表示 Bootloader 应进入 0502/0503 升级窗口。
 */
uint8_t boot_usart_update_requested(void)
{
    return boot_param_is_usart_request();
}

#if !defined(CMIS_BUILD_APP)
/* 发送 Bootloader 等待 App 启动的倒计时文本。
 * 参数：seconds 为倒计时秒数。
 * 返回：无。
 */
static void boot_usart_send_countdown_prompt(uint8_t seconds)
{
    char text[] = "wait for start Application(00s)......\r\n";

    text[27] = (char)('0' + (seconds / 10U));
    text[28] = (char)('0' + (seconds % 10U));
    if (seconds < 10U) {
        text[27] = (char)('0' + seconds);
        text[28] = 's';
        text[29] = ')';
        text[30] = '.';
        text[31] = '.';
        text[32] = '.';
        text[33] = '.';
        text[34] = '.';
        text[35] = '.';
        text[36] = '\r';
        text[37] = '\n';
        text[38] = '\0';
    }

    bsp_uart1_rs485_send_string(text);
}

/* 发送官方上位机 N-01 会识别的 Bootloader 启动提示。
 * 参数：无。
 * 返回：无。
 */
static void boot_usart_send_init_prompt(void)
{
    bsp_uart1_rs485_send_string("using command to interrupt start Application\r\n");
    boot_usart_send_countdown_prompt(10U);
}

/* 从 RS485 流中读取固件原始字节。
 * 参数：data 为输出字节；data_started 标记是否已经开始接收固件正文。
 * 返回：1 表示读到 1 字节，0 表示当前无数据或参数错误。
 */
static uint8_t boot_usart_read_firmware_byte(uint8_t *data, uint8_t *data_started)
{
    uint8_t byte;

    if ((data == 0) || (data_started == 0)) {
        return 0U;
    }

    while (bsp_uart1_rs485_read_byte(&byte) != 0U) {
        /* 固件开始前忽略上位机可能附带的空白字符，开始后所有字节都按固件内容处理。 */
        if ((*data_started == 0U) &&
            ((byte == '\r') || (byte == '\n') || (byte == ' ') || (byte == '\t'))) {
            continue;
        }

        *data_started = 1U;
        *data = byte;
        return 1U;
    }

    return 0U;
}

/* 接收 0502 后发送的官方固件包并暂存到内部 staging 区。
 * 参数：ready_ack_sent 输出是否已经提前回复过 0502 OK。
 * 返回：1 表示固件包校验并暂存成功，0 表示超时、溢出、魔术字错误、向量表错误或 Flash 写入失败。
 */
static uint8_t boot_usart_receive_firmware_to_staging(uint8_t *ready_ack_sent)
{
    uint8_t data;
    uint8_t data_started;
    uint8_t overflow;
    uint32_t start_tick;
    uint32_t last_rx_tick;
    uint32_t now_tick;
    uint32_t app_size;
    uint32_t crc;

    if (ready_ack_sent == 0) {
        return 0U;
    }

    s_usart_fw_ready = 0U;
    s_usart_fw_size = 0U;
    s_usart_fw_app_size = 0U;
    s_usart_fw_crc32 = 0U;
    s_usart_fw_stack = 0U;
    s_usart_fw_entry = 0U;
    overflow = 0U;
    data_started = 0U;
    *ready_ack_sent = 0U;

    boot_usart_frame_receiver_reset();

    start_tick = systick_get_tick();
    last_rx_tick = start_tick;

    while (1) {
        /* 接收阶段不按协议帧解析，而是把后续串口数据作为二进制固件流。 */
        if (boot_usart_read_firmware_byte(&data, &data_started) != 0U) {
            if (s_usart_fw_size < BOOT_USART_RAW_MAX_SIZE) {
                s_usart_fw_buffer[s_usart_fw_size] = data;
            } else {
                overflow = 1U;
            }
            s_usart_fw_size++;
            last_rx_tick = systick_get_tick();
            continue;
        }

        now_tick = systick_get_tick();
        /* 如果上位机在 0502 后稍晚才开始发文件，先给 OK，避免上位机一直等待。 */
        if ((s_usart_fw_size == 0U) &&
            (*ready_ack_sent == 0U) &&
            ((now_tick - start_tick) >= BOOT_USART_FW_READY_ACK_TIMEOUT_MS)) {
            *ready_ack_sent = 1U;
            boot_usart_send_ok(BOOT_USART_CMD_RECEIVE_BIN);
            boot_usart_frame_receiver_reset();
        }

        if ((s_usart_fw_size == 0U) && ((now_tick - start_tick) >= BOOT_USART_FW_FIRST_TIMEOUT_MS)) {
            return 0U;
        }

        if ((s_usart_fw_size != 0U) && ((now_tick - last_rx_tick) >= BOOT_USART_FW_IDLE_TIMEOUT_MS)) {
            break;
        }
    }

    if ((overflow != 0U) || (s_usart_fw_size < 12U) || (s_usart_fw_size > BOOT_USART_RAW_MAX_SIZE)) {
        return 0U;
    }

    /* 官方固件包前 4 字节为 5AA5C33C，后面才是真正 App bin。 */
    if (boot_usart_read_be32(s_usart_fw_buffer) != BOOT_FW_PACKAGE_MAGIC) {
        return 0U;
    }

    app_size = s_usart_fw_size - BOOT_USART_RAW_MAGIC_BYTES;
    if ((app_size == 0U) || (app_size > BOOT_APP_MAX_SIZE)) {
        return 0U;
    }

    s_usart_fw_stack = boot_usart_read_le32(&s_usart_fw_buffer[4]);
    s_usart_fw_entry = boot_usart_read_le32(&s_usart_fw_buffer[8]);
    /* 校验 App 向量表，防止把错误文件写入 App 区。 */
    if (boot_is_valid_app_vector(s_usart_fw_stack, s_usart_fw_entry) == 0U) {
        return 0U;
    }

    crc = boot_crc32_calc(&s_usart_fw_buffer[BOOT_USART_RAW_MAGIC_BYTES], app_size);
    if (boot_flash_erase(BOOT_FW_STAGING_ADDR, app_size) == 0U) {
        return 0U;
    }

    if (boot_flash_write(BOOT_FW_STAGING_ADDR, &s_usart_fw_buffer[BOOT_USART_RAW_MAGIC_BYTES], app_size) == 0U) {
        return 0U;
    }

    if (boot_flash_crc32(BOOT_FW_STAGING_ADDR, app_size) != crc) {
        return 0U;
    }

    s_usart_fw_app_size = app_size;
    s_usart_fw_crc32 = crc;
    s_usart_fw_ready = 1U;
    return 1U;
}

/* 计算串口升级后记录到参数区的下一个 App 版本序号。
 * 参数：无。
 * 返回：递增后的内部版本序号，参数区无效时根据是否已有 App 取初值。
 */
static uint32_t boot_usart_next_app_version(void)
{
    boot_param_t param;

    if (boot_param_load(&param) == 0U) {
        return boot_app_is_present() ? 2U : 1U;
    }

    if ((param.app_version == 0U) || (param.app_version == 0xFFFFFFFFU)) {
        return boot_app_is_present() ? 2U : 1U;
    }

    return param.app_version + 1U;
}

/* 将 staging 区中的串口固件应用到内部 App 区。
 * 参数：无。
 * 返回：BOOT_OK 表示写入和 CRC 校验成功，其余值表示镜像、擦写、CRC 或参数错误。
 */
static boot_status_t boot_usart_apply_staged_firmware(void)
{
    boot_image_header_t header;
    uint32_t remaining;
    uint32_t source_addr;
    uint32_t target_addr;
    uint32_t chunk;
    uint32_t index;

    if ((s_usart_fw_ready == 0U) ||
        (s_usart_fw_app_size == 0U) ||
        (s_usart_fw_app_size > BOOT_APP_MAX_SIZE)) {
        return BOOT_ERR_IMAGE_HEADER;
    }

    /* 正在擦写 App 区时关闭中断，避免中断服务访问正在变化的代码区域。 */
    __disable_irq();

    if (boot_flash_erase(BOOT_APP_START_ADDR, s_usart_fw_app_size) == 0U) {
        __enable_irq();
        return BOOT_ERR_FLASH_ERASE;
    }

    remaining = s_usart_fw_app_size;
    source_addr = BOOT_FW_STAGING_ADDR;
    target_addr = BOOT_APP_START_ADDR;

    while (remaining != 0U) {
        /* staging 位于内部 Flash，仍分块搬到 RAM 后再写入目标 App 区。 */
        chunk = remaining;
        if (chunk > BOOT_USART_COPY_BUFFER_SIZE) {
            chunk = BOOT_USART_COPY_BUFFER_SIZE;
        }

        for (index = 0U; index < chunk; index++) {
            s_usart_fw_buffer[index] = *((const uint8_t *)(source_addr + index));
        }

        if (boot_flash_write(target_addr, s_usart_fw_buffer, chunk) == 0U) {
            __enable_irq();
            return BOOT_ERR_FLASH_WRITE;
        }

        source_addr += chunk;
        target_addr += chunk;
        remaining -= chunk;
    }

    __enable_irq();

    if (boot_flash_crc32(BOOT_APP_START_ADDR, s_usart_fw_app_size) != s_usart_fw_crc32) {
        return BOOT_ERR_IMAGE_CRC;
    }

    header.magic = BOOT_IMAGE_MAGIC;
    header.header_size = sizeof(boot_image_header_t);
    header.header_crc32 = 0U;
    header.image_size = s_usart_fw_app_size;
    header.image_crc32 = s_usart_fw_crc32;
    header.image_version = boot_usart_next_app_version();
    header.target_addr = BOOT_APP_START_ADDR;
    header.stack_addr = s_usart_fw_stack;
    header.entry_addr = s_usart_fw_entry;
    header.state = BOOT_IMAGE_STATE_READY;
    header.sequence = 0U;
    header.tail_magic = BOOT_IMAGE_TAIL_MAGIC;
    for (index = 0U; index < (sizeof(header.reserved) / sizeof(header.reserved[0])); index++) {
        header.reserved[index] = 0U;
    }
    header.header_crc32 = boot_image_header_crc32(&header);

    /* App 已直接写入内部 Flash，这里只保存安装结果和版本信息。 */
    if (boot_param_mark_app_installed(&header) == 0U) {
        return BOOT_ERR_PARAM;
    }

    return BOOT_OK;
}

/* 等待上位机发送 0503，使已暂存固件正式生效。
 * 参数：无。
 * 返回：1 表示等待超时但保持 Bootloader 窗口，0 表示应用失败或已复位。
 */
static uint8_t boot_usart_wait_apply_command(void)
{
    protocol_frame_t frame;
    boot_usart_frame_status_t status;
    uint32_t start_tick;
    boot_status_t boot_status;

    start_tick = systick_get_tick();
    while ((systick_get_tick() - start_tick) < BOOT_USART_WAIT_0503_MS) {
        status = boot_usart_poll_frame(&frame);
        if (status == BOOT_USART_FRAME_NONE) {
            continue;
        }

        if (status == BOOT_USART_FRAME_ERROR) {
            boot_usart_send_error(PROTOCOL_CMD_ERROR);
            continue;
        }

        if (boot_usart_frame_matches_device(&frame) == 0U) {
            continue;
        }

        if ((frame.type == PROTOCOL_TYPE_COMMAND) &&
            (frame.command == BOOT_USART_CMD_APPLY_BIN) &&
            (frame.length == 0U)) {
            /* 先回复 0503 OK，再执行擦写，避免上位机因等待应答误判失败。 */
            boot_usart_send_ok(BOOT_USART_CMD_APPLY_BIN);
            delay_1ms(50U);
            boot_status = boot_usart_apply_staged_firmware();
            if (boot_status == BOOT_OK) {
                NVIC_SystemReset();
            }

            boot_usart_send_error(BOOT_USART_CMD_APPLY_BIN);
            return 0U;
        }

        boot_usart_handle_common_frame(&frame);
    }

    return 1U;
}

/* 处理 0502 接收固件命令。
 * 参数：无。
 * 返回：1 表示收包后等待 0503 超时，可继续回到升级窗口；0 表示收包失败。
 */
static uint8_t boot_usart_handle_receive_command(void)
{
    uint8_t ready_ack_sent;

    ready_ack_sent = 0U;
    if (boot_usart_receive_firmware_to_staging(&ready_ack_sent) == 0U) {
        boot_usart_send_error(BOOT_USART_CMD_RECEIVE_BIN);
        return 0U;
    }

    if (ready_ack_sent == 0U) {
        boot_usart_send_ok(BOOT_USART_CMD_RECEIVE_BIN);
    }
    return boot_usart_wait_apply_command();
}

/* Bootloader 串口升级窗口主循环。
 * 参数：无。
 * 返回：1 表示串口升级流程发生失败并停留 Bootloader，0 表示窗口超时可跳转 App。
 */
uint8_t boot_usart_bootloader_upgrade_window(void)
{
    protocol_frame_t frame;
    boot_usart_frame_status_t status;
    uint32_t start_tick;
    uint32_t window_timeout_ms;
    uint32_t elapsed;
    uint32_t last_init_prompt_tick;
    uint8_t prompt_7s_sent;
    uint8_t prompt_4s_sent;
    uint8_t prompt_1s_sent;
    uint8_t first_init_prompt_sent;
    uint8_t init_prompt_repeat_enabled;

    (void)boot_param_clear_update_request();
    boot_usart_frame_receiver_reset();

    prompt_7s_sent = 0U;
    prompt_4s_sent = 0U;
    prompt_1s_sent = 0U;
    first_init_prompt_sent = 0U;
    init_prompt_repeat_enabled = 1U;
    start_tick = systick_get_tick();
    last_init_prompt_tick = start_tick;
    window_timeout_ms = BOOT_USART_BOOT_WINDOW_MS;

    while ((systick_get_tick() - start_tick) < window_timeout_ms) {
        elapsed = systick_get_tick() - start_tick;

        /* 启动提示延迟发送，避开自动评测刚结束 0501 应答时的接收窗口。 */
        if (first_init_prompt_sent == 0U) {
            if (elapsed < BOOT_USART_INIT_PROMPT_FIRST_DELAY_MS) {
                continue;
            }

            first_init_prompt_sent = 1U;
            last_init_prompt_tick = systick_get_tick();
            boot_usart_send_init_prompt();
            continue;
        }

        if ((init_prompt_repeat_enabled != 0U) &&
            (elapsed < BOOT_USART_INIT_PROMPT_WINDOW_MS) &&
            ((systick_get_tick() - last_init_prompt_tick) >= BOOT_USART_INIT_PROMPT_REPEAT_MS)) {
            last_init_prompt_tick = systick_get_tick();
            boot_usart_send_init_prompt();
        }

        if ((elapsed >= 3000U) && (prompt_7s_sent == 0U)) {
            prompt_7s_sent = 1U;
            boot_usart_send_countdown_prompt(7U);
        }

        if ((elapsed >= 6000U) && (prompt_4s_sent == 0U)) {
            prompt_4s_sent = 1U;
            boot_usart_send_countdown_prompt(4U);
        }

        if ((elapsed >= 9000U) && (prompt_1s_sent == 0U)) {
            prompt_1s_sent = 1U;
            boot_usart_send_countdown_prompt(1U);
        }

        status = boot_usart_poll_frame(&frame);
        if (status == BOOT_USART_FRAME_NONE) {
            continue;
        }

        if (status == BOOT_USART_FRAME_ERROR) {
            boot_usart_send_error(PROTOCOL_CMD_ERROR);
            continue;
        }

        if (boot_usart_frame_matches_device(&frame) == 0U) {
            continue;
        }

        if ((frame.type == PROTOCOL_TYPE_COMMAND) &&
            (frame.command == BOOT_USART_CMD_RECEIVE_BIN) &&
            (frame.length == 0U)) {
            init_prompt_repeat_enabled = 0U;
            if (boot_usart_handle_receive_command() != 0U) {
                return 1U;
            }

            /* 错误固件后继续留在升级窗口，允许上位机重新发送 0502 和正确固件。 */
            start_tick = systick_get_tick();
            window_timeout_ms = BOOT_USART_WAIT_0503_MS;
            prompt_7s_sent = 1U;
            prompt_4s_sent = 1U;
            prompt_1s_sent = 1U;
            continue;
        }

        if ((frame.type == PROTOCOL_TYPE_COMMAND) &&
            (frame.command == BOOT_USART_CMD_ENTER_BOOT) &&
            (frame.length == 0U)) {
            boot_usart_send_ok(BOOT_USART_CMD_ENTER_BOOT);
            continue;
        }

        boot_usart_handle_common_frame(&frame);
    }

    return 0U;
}
#else
/* App 目标不包含 Bootloader 升级窗口，实现为空以保持接口一致。
 * 参数：无。
 * 返回：固定 0。
 */
uint8_t boot_usart_bootloader_upgrade_window(void)
{
    return 0U;
}
#endif
