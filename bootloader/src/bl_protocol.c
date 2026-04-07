/*
 * UART update protocol engine.
 *
 * Packet format (host → device):
 *   [SOF 0x5A] [CMD 1B] [LEN 2B LE] [DATA 0-256B] [CRC16 2B]
 *
 * Response (device → host):
 *   [SOF 0xA5] [STATUS 1B] [LEN 2B LE] [DATA 0-256B] [CRC16 2B]
 *
 * CRC16-CCITT (poly 0x1021, init 0xFFFF) is computed over the
 * CMD/STATUS + LEN + DATA fields (SOF is excluded).
 *
 * The LED toggles at 5 Hz while waiting for a command so the user
 * has visual confirmation the bootloader is alive.
 */

#include "bl_protocol.h"
#include "bl_uart.h"
#include "bl_flash.h"
#include "bl_crc32.h"
#include "bl_jump.h"
#include "bootloader.h"
#include "stm32g474xx.h"
#include "memory_map.h"
#include "image_header.h"

/* ------------------------------------------------------------------ */
/*  CRC16-CCITT (used only within this translation unit)              */
/* ------------------------------------------------------------------ */

static uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;
    for (int i = 0; i < 8; i++)
        crc = (crc & 0x8000U) ? ((crc << 1) ^ 0x1021U) : (crc << 1);
    return crc;
}

/* ------------------------------------------------------------------ */
/*  Packet I/O                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t  cmd;
    uint16_t length;
    uint8_t  data[PROTO_MAX_DATA];
} packet_t;

static int recv_packet(packet_t *pkt)
{
    uint8_t  byte;
    uint16_t crc = 0xFFFF;

    if (uart_recv_byte(&byte, BL_UART_TIMEOUT_MS) != 0)
        return BL_TIMEOUT;
    if (byte != PROTO_SOF_CMD)
        return BL_ERROR;

    /* CMD */
    if (uart_recv_byte(&pkt->cmd, BL_UART_TIMEOUT_MS) != 0)
        return BL_TIMEOUT;
    crc = crc16_update(crc, pkt->cmd);

    /* LENGTH (little-endian) */
    uint8_t lo, hi;
    if (uart_recv_byte(&lo, BL_UART_TIMEOUT_MS) != 0)  return BL_TIMEOUT;
    if (uart_recv_byte(&hi, BL_UART_TIMEOUT_MS) != 0)  return BL_TIMEOUT;
    crc = crc16_update(crc, lo);
    crc = crc16_update(crc, hi);
    pkt->length = (uint16_t)lo | ((uint16_t)hi << 8);

    if (pkt->length > PROTO_MAX_DATA)
        return BL_ERROR;

    /* DATA */
    for (uint16_t i = 0; i < pkt->length; i++) {
        if (uart_recv_byte(&pkt->data[i], BL_UART_TIMEOUT_MS) != 0)
            return BL_TIMEOUT;
        crc = crc16_update(crc, pkt->data[i]);
    }

    /* CRC16 */
    uint8_t crc_lo, crc_hi;
    if (uart_recv_byte(&crc_lo, BL_UART_TIMEOUT_MS) != 0)  return BL_TIMEOUT;
    if (uart_recv_byte(&crc_hi, BL_UART_TIMEOUT_MS) != 0)  return BL_TIMEOUT;
    uint16_t recv_crc = (uint16_t)crc_lo | ((uint16_t)crc_hi << 8);

    return (crc == recv_crc) ? BL_OK : BL_ERROR;
}

static void send_response(uint8_t status, const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    uart_send_byte(PROTO_SOF_RESP);

    uart_send_byte(status);
    crc = crc16_update(crc, status);

    uint8_t lo = (uint8_t)(len & 0xFFU);
    uint8_t hi = (uint8_t)(len >> 8);
    uart_send_byte(lo);
    uart_send_byte(hi);
    crc = crc16_update(crc, lo);
    crc = crc16_update(crc, hi);

    for (uint16_t i = 0; i < len; i++) {
        uart_send_byte(data[i]);
        crc = crc16_update(crc, data[i]);
    }

    uart_send_byte((uint8_t)(crc & 0xFFU));
    uart_send_byte((uint8_t)(crc >> 8));
}

/* ------------------------------------------------------------------ */
/*  Command handlers                                                  */
/* ------------------------------------------------------------------ */

static void handle_ping(void)
{
    send_response(STATUS_OK, NULL, 0);
}

static void handle_version(void)
{
    uint8_t ver[3] = { BL_VERSION_MAJOR, BL_VERSION_MINOR, BL_VERSION_PATCH };
    send_response(STATUS_OK, ver, 3);
}

static void handle_erase(void)
{
    uint8_t s = (flash_erase_app_region() == BL_OK) ? STATUS_OK : STATUS_ERR_ERASE;
    send_response(s, NULL, 0);
}

static void handle_write(const packet_t *pkt)
{
    if (pkt->length < 5) {
        send_response(STATUS_ERR_LEN, NULL, 0);
        return;
    }

    uint32_t offset = (uint32_t)pkt->data[0]
                    | ((uint32_t)pkt->data[1] << 8)
                    | ((uint32_t)pkt->data[2] << 16)
                    | ((uint32_t)pkt->data[3] << 24);

    uint32_t addr     = APP_FLASH_START + offset;
    uint16_t data_len = pkt->length - 4U;

    uint8_t s = (flash_write(addr, &pkt->data[4], data_len) == BL_OK)
                ? STATUS_OK : STATUS_ERR_WRITE;
    send_response(s, NULL, 0);
}

static void handle_verify(const packet_t *pkt)
{
    if (pkt->length < 8) {
        send_response(STATUS_ERR_LEN, NULL, 0);
        return;
    }

    uint32_t expected_crc = (uint32_t)pkt->data[0]
                          | ((uint32_t)pkt->data[1] << 8)
                          | ((uint32_t)pkt->data[2] << 16)
                          | ((uint32_t)pkt->data[3] << 24);

    uint32_t img_size = (uint32_t)pkt->data[4]
                      | ((uint32_t)pkt->data[5] << 8)
                      | ((uint32_t)pkt->data[6] << 16)
                      | ((uint32_t)pkt->data[7] << 24);

    const uint8_t *app = (const uint8_t *)(APP_FLASH_START + IMAGE_HEADER_SIZE);
    uint32_t computed  = crc32_compute(app, img_size);

    send_response((computed == expected_crc) ? STATUS_OK : STATUS_ERR_VERIFY,
                  NULL, 0);
}

static void handle_boot(void)
{
    const image_header_t *hdr = (const image_header_t *)APP_HEADER_ADDR;
    if (validate_image(hdr) != BL_OK) {
        send_response(STATUS_ERR_NO_APP, NULL, 0);
        return;
    }

    send_response(STATUS_OK, NULL, 0);

    /* Let the ACK transmit before we tear down the UART */
    uint32_t t = g_tick_ms;
    while ((g_tick_ms - t) < 50U)
        ;

    uart_deinit();
    jump_to_application(APP_VECTOR_ADDR);
}

/* ------------------------------------------------------------------ */
/*  Main protocol loop                                                */
/* ------------------------------------------------------------------ */

void protocol_run(void)
{
    packet_t pkt;
    uint32_t last_blink = g_tick_ms;

    while (1) {
        if ((g_tick_ms - last_blink) >= 200U) {
            GPIOx_ODR(LED_PORT) ^= (1UL << LED_PIN);
            last_blink = g_tick_ms;
        }

        int rc = recv_packet(&pkt);
        if (rc == BL_TIMEOUT)
            continue;
        if (rc != BL_OK) {
            send_response(STATUS_ERR_CRC, NULL, 0);
            continue;
        }

        switch (pkt.cmd) {
        case CMD_PING:    handle_ping();        break;
        case CMD_VERSION: handle_version();     break;
        case CMD_ERASE:   handle_erase();       break;
        case CMD_WRITE:   handle_write(&pkt);   break;
        case CMD_VERIFY:  handle_verify(&pkt);  break;
        case CMD_BOOT:    handle_boot();        break;
        default:
            send_response(STATUS_ERR_CMD, NULL, 0);
            break;
        }
    }
}
