#ifndef BL_PROTOCOL_H
#define BL_PROTOCOL_H

#include <stdint.h>

/* Packet framing */
#define PROTO_SOF_CMD       0x5AU
#define PROTO_SOF_RESP      0xA5U
#define PROTO_MAX_DATA      256U

/* Host → device commands */
#define CMD_PING            0x01U
#define CMD_VERSION         0x02U
#define CMD_ERASE           0x03U
#define CMD_WRITE           0x04U
#define CMD_VERIFY          0x05U
#define CMD_BOOT            0x06U
#define CMD_RESET           0x07U

/* Response status codes */
#define STATUS_OK           0x00U
#define STATUS_ERR_CMD      0x01U
#define STATUS_ERR_LEN      0x02U
#define STATUS_ERR_CRC      0x03U
#define STATUS_ERR_ERASE    0x04U
#define STATUS_ERR_WRITE    0x05U
#define STATUS_ERR_ADDR     0x06U
#define STATUS_ERR_VERIFY   0x07U
#define STATUS_ERR_NO_APP   0x08U

/* Main protocol loop — never returns on its own */
void protocol_run(void);

#endif /* BL_PROTOCOL_H */
