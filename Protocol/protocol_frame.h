#ifndef PROTOCOL_FRAME_H
#define PROTOCOL_FRAME_H

#include <stdint.h>

#define PROTOCOL_FRAME_START       0xA5B6U
#define PROTOCOL_FRAME_END         0xB6A5U
#define PROTOCOL_FRAME_VERSION     0x02U
#define PROTOCOL_DEVICE_BROADCAST  0xFFFFU
#define PROTOCOL_FRAME_MAX_PAYLOAD 255U
#define PROTOCOL_FRAME_MAX_BINARY  (13U + PROTOCOL_FRAME_MAX_PAYLOAD)
#define PROTOCOL_FRAME_MAX_ASCII   (PROTOCOL_FRAME_MAX_BINARY * 2U)

#define PROTOCOL_TYPE_COMMAND      0x01U
#define PROTOCOL_TYPE_RESPONSE     0x02U
#define PROTOCOL_TYPE_HEARTBEAT    0x05U
#define PROTOCOL_TYPE_ERROR        0xFFU

#define PROTOCOL_CMD_ERROR         0xEEEEU

typedef enum {
    PROTOCOL_STATUS_OK = 0,
    PROTOCOL_STATUS_ERR_PARAM,
    PROTOCOL_STATUS_ERR_SIZE,
    PROTOCOL_STATUS_ERR_MARK,
    PROTOCOL_STATUS_ERR_VERSION,
    PROTOCOL_STATUS_ERR_CRC,
} protocol_status_t;

typedef struct {
    uint16_t device_id;
    uint8_t type;
    uint16_t command;
    uint8_t length;
    uint8_t payload[PROTOCOL_FRAME_MAX_PAYLOAD];
} protocol_frame_t;

protocol_status_t protocol_frame_parse_ascii(const char *ascii, uint16_t ascii_len, protocol_frame_t *frame);
protocol_status_t protocol_frame_build_ascii(const protocol_frame_t *frame, char *ascii, uint16_t ascii_size, uint16_t *ascii_len);

#endif /* PROTOCOL_FRAME_H */
