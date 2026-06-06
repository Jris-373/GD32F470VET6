#include "protocol_frame.h"
#include "protocol_ascii_hex.h"
#include "protocol_crc16.h"

static uint16_t protocol_read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void protocol_write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
}

protocol_status_t protocol_frame_parse_ascii(const char *ascii, uint16_t ascii_len, protocol_frame_t *frame)
{
    uint8_t binary[PROTOCOL_FRAME_MAX_BINARY];
    uint16_t binary_len;
    uint16_t payload_len;
    uint16_t crc_in_frame;
    uint16_t crc_calc;
    uint16_t index;

    if ((ascii == 0) || (frame == 0)) {
        return PROTOCOL_STATUS_ERR_PARAM;
    }

    if (protocol_hex_decode(ascii, ascii_len, binary, sizeof(binary), &binary_len) != PROTOCOL_HEX_OK) {
        return PROTOCOL_STATUS_ERR_SIZE;
    }

    if (binary_len < 13U) {
        return PROTOCOL_STATUS_ERR_SIZE;
    }

    payload_len = binary[7];
    if (binary_len != (uint16_t)(13U + payload_len)) {
        return PROTOCOL_STATUS_ERR_SIZE;
    }

    if ((protocol_read_be16(&binary[0]) != PROTOCOL_FRAME_START) ||
        (protocol_read_be16(&binary[11U + payload_len]) != PROTOCOL_FRAME_END)) {
        return PROTOCOL_STATUS_ERR_MARK;
    }

    if (binary[8] != PROTOCOL_FRAME_VERSION) {
        return PROTOCOL_STATUS_ERR_VERSION;
    }

    crc_in_frame = protocol_read_be16(&binary[9U + payload_len]);
    crc_calc = protocol_crc16_modbus(binary, (uint16_t)(9U + payload_len));
    if (crc_in_frame != crc_calc) {
        return PROTOCOL_STATUS_ERR_CRC;
    }

    frame->device_id = protocol_read_be16(&binary[2]);
    frame->type = binary[4];
    frame->command = protocol_read_be16(&binary[5]);
    frame->length = (uint8_t)payload_len;

    for (index = 0U; index < payload_len; index++) {
        frame->payload[index] = binary[9U + index];
    }

    return PROTOCOL_STATUS_OK;
}

protocol_status_t protocol_frame_build_ascii(const protocol_frame_t *frame, char *ascii, uint16_t ascii_size, uint16_t *ascii_len)
{
    uint8_t binary[PROTOCOL_FRAME_MAX_BINARY];
    uint16_t binary_len;
    uint16_t crc;
    uint16_t index;

    if ((frame == 0) || (ascii == 0)) {
        return PROTOCOL_STATUS_ERR_PARAM;
    }

    binary_len = (uint16_t)(13U + frame->length);
    if ((frame->length > PROTOCOL_FRAME_MAX_PAYLOAD) || (ascii_size < (uint16_t)(binary_len * 2U))) {
        return PROTOCOL_STATUS_ERR_SIZE;
    }

    protocol_write_be16(&binary[0], PROTOCOL_FRAME_START);
    protocol_write_be16(&binary[2], frame->device_id);
    binary[4] = frame->type;
    protocol_write_be16(&binary[5], frame->command);
    binary[7] = frame->length;
    binary[8] = PROTOCOL_FRAME_VERSION;

    for (index = 0U; index < frame->length; index++) {
        binary[9U + index] = frame->payload[index];
    }

    crc = protocol_crc16_modbus(binary, (uint16_t)(9U + frame->length));
    protocol_write_be16(&binary[9U + frame->length], crc);
    protocol_write_be16(&binary[11U + frame->length], PROTOCOL_FRAME_END);

    if (protocol_hex_encode(binary, binary_len, ascii, ascii_size, ascii_len) != PROTOCOL_HEX_OK) {
        return PROTOCOL_STATUS_ERR_SIZE;
    }

    return PROTOCOL_STATUS_OK;
}
