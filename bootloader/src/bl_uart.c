/*
 * UART driver — register-level USART2 on PA2/PA3 (Nucleo VCP).
 *
 * No interrupts — the bootloader polls TXE/RXNE.  Timeouts are
 * derived from the SysTick millisecond counter (g_tick_ms).
 */

#include "bl_uart.h"
#include "bootloader.h"
#include "stm32g474xx.h"

#define HSI_FREQ    16000000UL

void uart_init(uint32_t baud)
{
    /* PA2 = USART2_TX, PA3 = USART2_RX — both AF7 */
    uint32_t moder = GPIOx_MODER(GPIOA_BASE);
    moder &= ~((3UL << (2U * 2)) | (3UL << (3U * 2)));
    moder |=   (GPIO_MODER_AF << (2U * 2)) | (GPIO_MODER_AF << (3U * 2));
    GPIOx_MODER(GPIOA_BASE) = moder;

    uint32_t afrl = GPIOx_AFRL(GPIOA_BASE);
    afrl &= ~((0xFUL << (2U * 4)) | (0xFUL << (3U * 4)));
    afrl |=   (7UL   << (2U * 4)) | (7UL   << (3U * 4));
    GPIOx_AFRL(GPIOA_BASE) = afrl;

    /* Disable USART before touching configuration registers */
    USARTx_CR1(USART2_BASE) = 0;

    /* Baud rate: BRR = f_ck / baud  (integer divider, < 0.1 % error) */
    USARTx_BRR(USART2_BASE) = (HSI_FREQ + baud / 2U) / baud;

    /* 8-N-1, no hardware flow control */
    USARTx_CR2(USART2_BASE) = 0;
    USARTx_CR3(USART2_BASE) = 0;

    /* Enable USART, transmitter, receiver */
    USARTx_CR1(USART2_BASE) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_deinit(void)
{
    while (!(USARTx_ISR(USART2_BASE) & USART_ISR_TC))
        ;
    USARTx_CR1(USART2_BASE) = 0;
}

void uart_send_byte(uint8_t byte)
{
    while (!(USARTx_ISR(USART2_BASE) & USART_ISR_TXE))
        ;
    USARTx_TDR(USART2_BASE) = byte;
}

int uart_recv_byte(uint8_t *byte, uint32_t timeout_ms)
{
    uint32_t start = g_tick_ms;

    while ((g_tick_ms - start) < timeout_ms) {
        /* Clear any overrun so the peripheral keeps receiving */
        if (USARTx_ISR(USART2_BASE) & USART_ISR_ORE)
            USARTx_ICR(USART2_BASE) = USART_ICR_ORECF;

        if (USARTx_ISR(USART2_BASE) & USART_ISR_RXNE) {
            *byte = (uint8_t)USARTx_RDR(USART2_BASE);
            return 0;
        }
    }

    return -1;
}

void uart_send(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        uart_send_byte(data[i]);
}

void uart_puts(const char *s)
{
    while (*s)
        uart_send_byte((uint8_t)*s++);
}
