/*
 * Image validation and jump-to-application.
 *
 * validate_image() checks magic, CRC32, size sanity, and MSP range.
 * jump_to_application() tears down bootloader peripherals, relocates
 * VTOR, reloads MSP, and branches to the app's reset vector.
 *
 * The DSB/ISB pair after writing VTOR is critical: without them the
 * core may fetch stale vectors from the old table.
 */

#include "bl_jump.h"
#include "bl_crc32.h"
#include "bootloader.h"
#include "stm32g474xx.h"
#include "memory_map.h"
#include "image_header.h"

int validate_image(const image_header_t *hdr)
{
    if (hdr->magic != IMAGE_MAGIC)
        return BL_ERROR;

    if (hdr->header_version != IMAGE_HEADER_VERSION)
        return BL_ERROR;

    if (hdr->image_size == 0 || hdr->image_size == 0xFFFFFFFFUL)
        return BL_ERROR;

    if (hdr->image_size > (APP_FLASH_SIZE - IMAGE_HEADER_SIZE))
        return BL_ERROR;

    if (hdr->vector_table_offset != IMAGE_HEADER_SIZE)
        return BL_ERROR;

    /* CRC32 covers only the application bytes after the header */
    const uint8_t *app_data = (const uint8_t *)hdr + IMAGE_HEADER_SIZE;
    uint32_t computed = crc32_compute(app_data, hdr->image_size);
    if (computed != hdr->crc32)
        return BL_ERROR;

    /*
     * Sanity-check the initial MSP in the app vector table.
     * It must point somewhere inside SRAM.
     */
    uint32_t app_msp = *(volatile uint32_t *)APP_VECTOR_ADDR;
    if (app_msp < SRAM_BASE_ADDR || app_msp > (SRAM_BASE_ADDR + SRAM_SIZE))
        return BL_ERROR;

    /* Reset vector must be Thumb (bit 0 set) and within app flash */
    uint32_t app_reset = *(volatile uint32_t *)(APP_VECTOR_ADDR + 4U);
    if (!(app_reset & 1UL))
        return BL_ERROR;
    if ((app_reset & ~1UL) < APP_VECTOR_ADDR || (app_reset & ~1UL) >= APP_FLASH_END)
        return BL_ERROR;

    return BL_OK;
}

__attribute__((noreturn))
void jump_to_application(uint32_t vector_table_addr)
{
    __disable_irq();

    /* Disable SysTick */
    SYSTICK_CTRL = 0;
    SYSTICK_LOAD = 0;
    SYSTICK_VAL  = 0;

    /* Clear all NVIC enable and pending bits (4 regs covers 128 IRQs) */
    for (int i = 0; i < 8; i++) {
        NVIC_ICER(i) = 0xFFFFFFFFUL;
        NVIC_ICPR(i) = 0xFFFFFFFFUL;
    }

    /* Relocate vector table to the application */
    SCB_VTOR = vector_table_addr;
    __DSB();
    __ISB();

    uint32_t app_msp   = *(volatile uint32_t *)(vector_table_addr);
    uint32_t app_reset = *(volatile uint32_t *)(vector_table_addr + 4U);

    __set_MSP(app_msp);
    __enable_irq();

    ((void (*)(void))app_reset)();

    while (1);
}
