/*
 * C-based startup for the application.
 *
 * Structurally identical to the bootloader startup.  The linker
 * script places this vector table at 0x0801 0200 (after the 512-byte
 * image header), and the bootloader sets VTOR to this address before
 * jumping here.
 */

#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

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
 * 16 system exceptions + 112 peripheral IRQ slots.
 * Padding to 128 entries ensures stray interrupts land in
 * Default_Handler instead of fetching garbage addresses.
 */
__attribute__((section(".isr_vector"), used))
const uint32_t g_vector_table[128] = {
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0, 0, 0, 0,
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
    /* 16-127: peripheral IRQs — zero-init */
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    main();
    while (1);
}

void Default_Handler(void)
{
    while (1);
}
