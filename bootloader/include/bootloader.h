#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

/* Bootloader version */
#define BL_VERSION_MAJOR    1
#define BL_VERSION_MINOR    0
#define BL_VERSION_PATCH    0
#define BL_VERSION_STR      "1.0.0"

/* UART */
#define BL_UART_BAUD        115200UL
#define BL_UART_TIMEOUT_MS  1000UL

/* Hardware pins — Nucleo-G474RE */
#define LED_PORT            GPIOA_BASE
#define LED_PIN             5U
#define BOOT_PIN_PORT       GPIOC_BASE
#define BOOT_PIN            13U

/* Generic return codes */
#define BL_OK               0
#define BL_ERROR            (-1)
#define BL_TIMEOUT          (-2)

/* SysTick millisecond counter (incremented by SysTick_Handler) */
extern volatile uint32_t g_tick_ms;
void     system_init(void);

#endif /* BOOTLOADER_H */
