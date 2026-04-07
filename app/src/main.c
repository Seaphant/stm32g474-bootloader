/*
 * Demo application — LED blink + UART banner.
 *
 * Proves that the bootloader correctly:
 *   • relocated VTOR
 *   • loaded the application MSP
 *   • branched to this reset handler
 *
 * The image header lives in the .image_header section so that
 * pack_image.py can patch crc32 and image_size post-link.
 */

#include "app.h"
#include "stm32g474xx.h"
#include "memory_map.h"
#include "image_header.h"

#define LED_PORT    GPIOA_BASE
#define LED_PIN     5U

extern void system_init(void);

/* Placed at the very start of app flash (0x0801 0000) */
__attribute__((section(".image_header"), used))
const image_header_t app_image_header = {
    .magic               = IMAGE_MAGIC,
    .header_version      = IMAGE_HEADER_VERSION,
    .image_size          = 0xFFFFFFFFUL,        /* patched post-link */
    .crc32               = 0xFFFFFFFFUL,        /* patched post-link */
    .version_major       = APP_VERSION_MAJOR,
    .version_minor       = APP_VERSION_MINOR,
    .version_patch       = APP_VERSION_PATCH,
    .vector_table_offset = IMAGE_HEADER_SIZE,
};

/* ---- Minimal UART helpers (no library dependency) ---- */

static void uart_putc(uint8_t c)
{
    while (!(USARTx_ISR(USART2_BASE) & USART_ISR_TXE))
        ;
    USARTx_TDR(USART2_BASE) = c;
}

static void uart_puts(const char *s)
{
    while (*s)
        uart_putc((uint8_t)*s++);
}

static void uart_init_simple(void)
{
    uint32_t moder = GPIOx_MODER(GPIOA_BASE);
    moder &= ~((3UL << (2U * 2)) | (3UL << (3U * 2)));
    moder |=   (GPIO_MODER_AF << (2U * 2)) | (GPIO_MODER_AF << (3U * 2));
    GPIOx_MODER(GPIOA_BASE) = moder;

    uint32_t afrl = GPIOx_AFRL(GPIOA_BASE);
    afrl &= ~((0xFUL << (2U * 4)) | (0xFUL << (3U * 4)));
    afrl |=   (7UL   << (2U * 4)) | (7UL   << (3U * 4));
    GPIOx_AFRL(GPIOA_BASE) = afrl;

    USARTx_CR1(USART2_BASE) = 0;
    USARTx_BRR(USART2_BASE) = 139;     /* 16 MHz / 115200 ≈ 139 */
    USARTx_CR1(USART2_BASE) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ---- Busy-wait delay ---- */

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
        for (volatile uint32_t j = 0; j < 2000U; j++)
            ;
}

/* ---- Entry point ---- */

int main(void)
{
    system_init();

    /* PA5 → output (LED) */
    uint32_t m = GPIOx_MODER(GPIOA_BASE);
    m &= ~(3UL << (LED_PIN * 2));
    m |=  (GPIO_MODER_OUTPUT << (LED_PIN * 2));
    GPIOx_MODER(GPIOA_BASE) = m;

    uart_init_simple();
    uart_puts("\r\n--- Application v");
    uart_putc('0' + APP_VERSION_MAJOR);
    uart_putc('.');
    uart_putc('0' + APP_VERSION_MINOR);
    uart_putc('.');
    uart_putc('0' + APP_VERSION_PATCH);
    uart_puts(" running ---\r\n");

    while (1) {
        GPIOx_ODR(GPIOA_BASE) ^= (1UL << LED_PIN);
        delay_ms(500);
    }
}
