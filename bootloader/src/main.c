/*
 * Bootloader entry point — Milestone 1: UART hello + LED blink.
 *
 * Purpose:
 *   Verify the MCU boots from the bootloader flash region (0x08000000),
 *   the clock tree is running (SysTick ticks), the UART transmits
 *   correctly, and GPIO toggles the LED.
 *
 * Later milestones will progressively add:
 *   M3 — jump-to-application
 *   M5 — image validation (magic + CRC32)
 *   M7 — UART update protocol
 */

#include "bootloader.h"
#include "bl_uart.h"
#include "stm32g474xx.h"

/* ------------------------------------------------------------------ */

static void gpio_init(void)
{
    /* PA5 → push-pull output (LED LD2 on Nucleo-G474RE) */
    uint32_t m = GPIOx_MODER(GPIOA_BASE);
    m &= ~(3UL << (LED_PIN * 2));
    m |=  (GPIO_MODER_OUTPUT << (LED_PIN * 2));
    GPIOx_MODER(GPIOA_BASE) = m;
}

static void led_toggle(void)
{
    GPIOx_ODR(LED_PORT) ^= (1UL << LED_PIN);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    system_init();
    gpio_init();
    uart_init(BL_UART_BAUD);

    uart_puts("\r\n");
    uart_puts("=== STM32G474 Bootloader v" BL_VERSION_STR " ===\r\n");
    uart_puts("[BL] Milestone 1 — boot OK\r\n");
    uart_puts("[BL] UART @ 115200, HSI16, SysTick 1 ms\r\n");

    while (1) {
        led_toggle();
        uint32_t t = g_tick_ms;
        while ((g_tick_ms - t) < 500U)
            ;
    }
}
