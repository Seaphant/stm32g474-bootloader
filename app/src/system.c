/*
 * Application system init.
 *
 * Runs on HSI16 for simplicity.  In a production firmware you would
 * configure the PLL here (e.g. 170 MHz from HSI via PLL) and set the
 * corresponding flash wait-states.
 */

#include "stm32g474xx.h"

void system_init(void)
{
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY))
        ;

    FLASH_ACR = (FLASH_ACR & ~0xFUL) | 0UL;

    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC_AHB2ENR;
    (void)RCC_APB1ENR1;
}
