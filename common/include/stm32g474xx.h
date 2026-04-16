#ifndef STM32G474XX_H
#define STM32G474XX_H

/*
 * Minimal register definitions for the STM32G474.
 * Covers only the peripherals used by the bootloader and demo application.
 * Register addresses verified against RM0440 Rev 8.
 */

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Cortex-M4 System Registers                                        */
/* ------------------------------------------------------------------ */

#define SCB_VTOR            (*(volatile uint32_t *)0xE000ED08UL)
#define SCB_AIRCR           (*(volatile uint32_t *)0xE000ED0CUL)

#define SYSTICK_CTRL        (*(volatile uint32_t *)0xE000E010UL)
#define SYSTICK_LOAD        (*(volatile uint32_t *)0xE000E014UL)
#define SYSTICK_VAL         (*(volatile uint32_t *)0xE000E018UL)

#define SYSTICK_CTRL_ENABLE     (1UL << 0)
#define SYSTICK_CTRL_TICKINT    (1UL << 1)
#define SYSTICK_CTRL_CLKSRC     (1UL << 2)

#define NVIC_ICER(n)        (*(volatile uint32_t *)(0xE000E180UL + 4U * (n)))
#define NVIC_ICPR(n)        (*(volatile uint32_t *)(0xE000E280UL + 4U * (n)))

/* ------------------------------------------------------------------ */
/*  RCC – Reset and Clock Control (base 0x4002 1000)                  */
/* ------------------------------------------------------------------ */

#define RCC_BASE                0x40021000UL
#define RCC_CR                  (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define RCC_CFGR                (*(volatile uint32_t *)(RCC_BASE + 0x08UL))
#define RCC_AHB2ENR             (*(volatile uint32_t *)(RCC_BASE + 0x4CUL))
#define RCC_APB1ENR1            (*(volatile uint32_t *)(RCC_BASE + 0x58UL))

#define RCC_CR_HSION            (1UL << 8)
#define RCC_CR_HSIRDY           (1UL << 10)

#define RCC_AHB2ENR_GPIOAEN     (1UL << 0)
#define RCC_AHB2ENR_GPIOCEN     (1UL << 2)

#define RCC_APB1ENR1_USART2EN   (1UL << 17)

/* ------------------------------------------------------------------ */
/*  FLASH (base 0x4002 2000)                                          */
/* ------------------------------------------------------------------ */

#define FLASH_R_BASE            0x40022000UL
#define FLASH_ACR               (*(volatile uint32_t *)(FLASH_R_BASE + 0x00UL))
#define FLASH_KEYR              (*(volatile uint32_t *)(FLASH_R_BASE + 0x08UL))
#define FLASH_SR                (*(volatile uint32_t *)(FLASH_R_BASE + 0x10UL))
#define FLASH_CR                (*(volatile uint32_t *)(FLASH_R_BASE + 0x14UL))

#define FLASH_KEY1              0x45670123UL
#define FLASH_KEY2              0xCDEF89ABUL

#define FLASH_CR_PG             (1UL << 0)
#define FLASH_CR_PER            (1UL << 1)
#define FLASH_CR_MER1           (1UL << 2)
#define FLASH_CR_PNB_POS        3U
#define FLASH_CR_PNB_MSK        (0xFFUL << FLASH_CR_PNB_POS)
#define FLASH_CR_STRT           (1UL << 16)
#define FLASH_CR_LOCK           (1UL << 31)

#define FLASH_SR_EOP            (1UL << 0)
#define FLASH_SR_OPERR          (1UL << 1)
#define FLASH_SR_PROGERR        (1UL << 3)
#define FLASH_SR_WRPERR         (1UL << 4)
#define FLASH_SR_PGAERR         (1UL << 5)
#define FLASH_SR_SIZERR         (1UL << 6)
#define FLASH_SR_PGSERR         (1UL << 7)
#define FLASH_SR_BSY            (1UL << 16)

#define FLASH_SR_ERR_MASK       (FLASH_SR_OPERR  | FLASH_SR_PROGERR | \
                                 FLASH_SR_WRPERR | FLASH_SR_PGAERR  | \
                                 FLASH_SR_SIZERR | FLASH_SR_PGSERR)

/* ------------------------------------------------------------------ */
/*  GPIO (GPIOA base 0x4800 0000, GPIOC base 0x4800 0800)            */
/* ------------------------------------------------------------------ */

#define GPIOA_BASE              0x48000000UL
#define GPIOC_BASE              0x48000800UL

#define GPIOx_MODER(base)       (*(volatile uint32_t *)((base) + 0x00UL))
#define GPIOx_OTYPER(base)      (*(volatile uint32_t *)((base) + 0x04UL))
#define GPIOx_OSPEEDR(base)     (*(volatile uint32_t *)((base) + 0x08UL))
#define GPIOx_PUPDR(base)       (*(volatile uint32_t *)((base) + 0x0CUL))
#define GPIOx_IDR(base)         (*(volatile uint32_t *)((base) + 0x10UL))
#define GPIOx_ODR(base)         (*(volatile uint32_t *)((base) + 0x14UL))
#define GPIOx_BSRR(base)        (*(volatile uint32_t *)((base) + 0x18UL))
#define GPIOx_AFRL(base)        (*(volatile uint32_t *)((base) + 0x20UL))
#define GPIOx_AFRH(base)        (*(volatile uint32_t *)((base) + 0x24UL))

#define GPIO_MODER_INPUT        0x0UL
#define GPIO_MODER_OUTPUT       0x1UL
#define GPIO_MODER_AF           0x2UL
#define GPIO_MODER_ANALOG       0x3UL

/* ------------------------------------------------------------------ */
/*  USART2 (base 0x4000 4400)                                        */
/* ------------------------------------------------------------------ */

#define USART2_BASE             0x40004400UL

#define USARTx_CR1(base)        (*(volatile uint32_t *)((base) + 0x00UL))
#define USARTx_CR2(base)        (*(volatile uint32_t *)((base) + 0x04UL))
#define USARTx_CR3(base)        (*(volatile uint32_t *)((base) + 0x08UL))
#define USARTx_BRR(base)        (*(volatile uint32_t *)((base) + 0x0CUL))
#define USARTx_ISR(base)        (*(volatile uint32_t *)((base) + 0x1CUL))
#define USARTx_ICR(base)        (*(volatile uint32_t *)((base) + 0x20UL))
#define USARTx_RDR(base)        (*(volatile uint32_t *)((base) + 0x24UL))
#define USARTx_TDR(base)        (*(volatile uint32_t *)((base) + 0x28UL))

#define USART_CR1_UE            (1UL << 0)
#define USART_CR1_RE            (1UL << 2)
#define USART_CR1_TE            (1UL << 3)

#define USART_ISR_FE            (1UL << 1)
#define USART_ISR_NE            (1UL << 2)
#define USART_ISR_ORE           (1UL << 3)
#define USART_ISR_RXNE          (1UL << 5)
#define USART_ISR_TC            (1UL << 6)
#define USART_ISR_TXE           (1UL << 7)

#define USART_ICR_FECF          (1UL << 1)
#define USART_ICR_NECF          (1UL << 2)
#define USART_ICR_ORECF         (1UL << 3)

/* ------------------------------------------------------------------ */
/*  Inline-assembly helpers (replace CMSIS intrinsics)                */
/* ------------------------------------------------------------------ */

static inline void __disable_irq(void)
{
    __asm volatile ("cpsid i" ::: "memory");
}

static inline void __enable_irq(void)
{
    __asm volatile ("cpsie i" ::: "memory");
}

static inline void __set_MSP(uint32_t msp)
{
    __asm volatile ("MSR msp, %0" :: "r" (msp) : "memory");
}

static inline void __DSB(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
}

static inline void __ISB(void)
{
    __asm volatile ("isb 0xF" ::: "memory");
}

#endif /* STM32G474XX_H */
