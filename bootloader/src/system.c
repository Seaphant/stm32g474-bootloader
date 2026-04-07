/*
 * System initialisation — clock tree and SysTick.
 *
 * The bootloader runs on the 16 MHz HSI oscillator (default after
 * reset).  No PLL is configured; this keeps startup deterministic
 * and avoids dependencies on external crystals.
 *
 * SysTick provides a 1 ms time base used by the UART timeout logic.
 */

#include "stm32g474xx.h"
#include "bootloader.h"

volatile uint32_t g_tick_ms;

/* Overrides the weak alias in startup.c */
void SysTick_Handler(void)
{
    g_tick_ms++;
}

void system_init(void)
{
    /* Ensure HSI16 is on and stable */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY))
        ;

    /* 0 wait-states at 16 MHz, VOS range 1 */
    FLASH_ACR = (FLASH_ACR & ~0xFUL) | 0UL;

    /* Enable clocks for the peripherals we touch */
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* SysTick — 1 ms tick from the 16 MHz processor clock */
    SYSTICK_LOAD = 16000UL - 1;
    SYSTICK_VAL  = 0;
    SYSTICK_CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_TICKINT |
                   SYSTICK_CTRL_CLKSRC;
}
