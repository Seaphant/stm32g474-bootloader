#ifndef BL_UART_H
#define BL_UART_H

#include <stdint.h>

void uart_init(uint32_t baud);
void uart_deinit(void);
void uart_send_byte(uint8_t byte);
int  uart_recv_byte(uint8_t *byte, uint32_t timeout_ms);
void uart_send(const uint8_t *data, uint32_t len);
void uart_puts(const char *s);

#endif /* BL_UART_H */
