/*
 * C-based startup for the bootloader.
 *
 * Defines the Cortex-M4 vector table as a const array and a
 * Reset_Handler that copies .data, zeroes .bss, then calls main().
 * All exception handlers default to an infinite loop (safe halt).
 */

#include <stdint.h>

/* Symbols exported by the linker script */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Cortex-M4 system exceptions — weak so the app or drivers can override */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/*
 * Vector table.  Entry 0 is the initial MSP; entry 1 is the reset
 * handler.  Remaining entries cover the standard Cortex-M exceptions.
 * Peripheral IRQs are unused in the bootloader — any stray interrupt
 * will land in Default_Handler.
 */
__attribute__((section(".isr_vector"), used))
const uint32_t g_vector_table[] = {
    (uint32_t)&_estack,             /*  0  Initial MSP              */
    (uint32_t)Reset_Handler,        /*  1  Reset                    */
    (uint32_t)NMI_Handler,          /*  2  NMI                      */
    (uint32_t)HardFault_Handler,    /*  3  HardFault                */
    (uint32_t)MemManage_Handler,    /*  4  MemManage                */
    (uint32_t)BusFault_Handler,     /*  5  BusFault                 */
    (uint32_t)UsageFault_Handler,   /*  6  UsageFault               */
    0, 0, 0, 0,                     /*  7-10  Reserved              */
    (uint32_t)SVC_Handler,          /* 11  SVCall                   */
    (uint32_t)DebugMon_Handler,     /* 12  Debug Monitor            */
    0,                              /* 13  Reserved                 */
    (uint32_t)PendSV_Handler,       /* 14  PendSV                   */
    (uint32_t)SysTick_Handler,      /* 15  SysTick                  */
};

void Reset_Handler(void)
{
    /* Copy .data initializers from flash to SRAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    /* Zero-fill .bss */
    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    main();

    /* Trap: main() should never return */
    while (1);
}

void Default_Handler(void)
{
    while (1);
}
