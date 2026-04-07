/*
 * Bootloader entry point.
 *
 * Boot-decision flow:
 *   1. Init clock, GPIO (LED + boot pin), UART, SysTick
 *   2. Read boot pin (PC13 — user button B1, active-low on Nucleo)
 *   3. If button NOT pressed → validate application image
 *      - Valid   → tear down peripherals, jump to app
 *      - Invalid → fall through to update mode
 *   4. If button pressed → enter update mode directly
 *
 * Update mode runs protocol_run(), which handles erase / write /
 * verify / boot commands over UART.
 */

#include "bootloader.h"
#include "bl_uart.h"
#include "bl_jump.h"
#include "bl_protocol.h"
#include "stm32g474xx.h"
#include "memory_map.h"
#include "image_header.h"

/* ------------------------------------------------------------------ */

static void gpio_init(void)
{
    /* PA5 → push-pull output (LED LD2 on Nucleo-G474RE) */
    uint32_t m = GPIOx_MODER(GPIOA_BASE);
    m &= ~(3UL << (LED_PIN * 2));
    m |=  (GPIO_MODER_OUTPUT << (LED_PIN * 2));
    GPIOx_MODER(GPIOA_BASE) = m;

    /* PC13 → digital input with pull-up (user button B1) */
    m = GPIOx_MODER(BOOT_PIN_PORT);
    m &= ~(3UL << (BOOT_PIN * 2));
    GPIOx_MODER(BOOT_PIN_PORT) = m;

    uint32_t pupdr = GPIOx_PUPDR(BOOT_PIN_PORT);
    pupdr &= ~(3UL << (BOOT_PIN * 2));
    pupdr |=  (1UL << (BOOT_PIN * 2));
    GPIOx_PUPDR(BOOT_PIN_PORT) = pupdr;
}

static int boot_pin_pressed(void)
{
    return !(GPIOx_IDR(BOOT_PIN_PORT) & (1UL << BOOT_PIN));
}

/* ------------------------------------------------------------------ */

int main(void)
{
    system_init();
    gpio_init();
    uart_init(BL_UART_BAUD);

    uart_puts("\r\n=== STM32G474 Bootloader v" BL_VERSION_STR " ===\r\n");

    if (!boot_pin_pressed()) {
        const image_header_t *hdr = (const image_header_t *)APP_HEADER_ADDR;

        uart_puts("[BL] Validating application image...\r\n");

        if (validate_image(hdr) == BL_OK) {
            uart_puts("[BL] Image valid — jumping to application\r\n");
            uart_deinit();
            jump_to_application(APP_VECTOR_ADDR);
        }

        uart_puts("[BL] No valid image — entering update mode\r\n");
    } else {
        uart_puts("[BL] Boot pin held — forced update mode\r\n");
    }

    uart_puts("[BL] Waiting for host (UART protocol)...\r\n");
    protocol_run();

    while (1)
        ;
}
