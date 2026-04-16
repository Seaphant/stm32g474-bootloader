/*
 * Flash driver — page erase and double-word programming.
 *
 * The STM32G4 flash requires 8-byte-aligned, double-word writes.
 * Partial trailing bytes are padded with 0xFF so erased cells are
 * left unchanged.
 *
 * All write operations are bounds-checked against the application
 * region; the bootloader pages can never be overwritten from here.
 */

#include "bl_flash.h"
#include "bootloader.h"
#include "stm32g474xx.h"
#include "memory_map.h"

/* ------------------------------------------------------------------ */

#define FLASH_TIMEOUT_CYCLES  1000000UL

static int flash_wait_bsy(void)
{
    uint32_t count = FLASH_TIMEOUT_CYCLES;
    while (FLASH_SR & FLASH_SR_BSY) {
        if (--count == 0)
            return BL_TIMEOUT;
    }
    return BL_OK;
}

static void flash_clear_errors(void)
{
    FLASH_SR = FLASH_SR_ERR_MASK | FLASH_SR_EOP;
}

/* ------------------------------------------------------------------ */

int flash_unlock(void)
{
    if (!(FLASH_CR & FLASH_CR_LOCK))
        return BL_OK;

    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;

    return (FLASH_CR & FLASH_CR_LOCK) ? BL_ERROR : BL_OK;
}

void flash_lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

int flash_erase_page(uint32_t page)
{
    if (page < APP_PAGE_START)
        return BL_ERROR;
    if (page >= APP_PAGE_START + APP_PAGE_COUNT)
        return BL_ERROR;

    if (flash_wait_bsy() != BL_OK)
        return BL_TIMEOUT;
    flash_clear_errors();

    FLASH_CR &= ~FLASH_CR_PNB_MSK;
    FLASH_CR |= (page << FLASH_CR_PNB_POS) | FLASH_CR_PER;
    FLASH_CR |= FLASH_CR_STRT;

    if (flash_wait_bsy() != BL_OK) {
        FLASH_CR &= ~FLASH_CR_PER;
        return BL_TIMEOUT;
    }

    if (FLASH_SR & FLASH_SR_ERR_MASK) {
        FLASH_CR &= ~FLASH_CR_PER;
        return BL_ERROR;
    }

    FLASH_SR  = FLASH_SR_EOP;
    FLASH_CR &= ~FLASH_CR_PER;
    return BL_OK;
}

int flash_erase_app_region(void)
{
    if (flash_unlock() != BL_OK)
        return BL_ERROR;

    for (uint32_t p = APP_PAGE_START; p < APP_PAGE_START + APP_PAGE_COUNT; p++) {
        if (flash_erase_page(p) != BL_OK) {
            flash_lock();
            return BL_ERROR;
        }
    }

    flash_lock();
    return BL_OK;
}

int flash_write(uint32_t address, const uint8_t *data, uint32_t len)
{
    if (address & 0x7UL)
        return BL_ERROR;
    if (address < APP_FLASH_START || (address + len) > APP_FLASH_END)
        return BL_ERROR;

    if (flash_unlock() != BL_OK)
        return BL_ERROR;

    flash_clear_errors();

    uint32_t pos = 0;
    while (pos < len) {
        /*
         * Build an 8-byte buffer.  Unwritten tail bytes stay 0xFF
         * so erased flash cells are preserved.
         */
        uint8_t buf[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        uint32_t remaining = len - pos;
        uint32_t chunk = (remaining < 8U) ? remaining : 8U;
        for (uint32_t i = 0; i < chunk; i++)
            buf[i] = data[pos + i];

        /* Assemble two 32-bit words (avoids strict-aliasing UB) */
        uint32_t word_lo = (uint32_t)buf[0]       | ((uint32_t)buf[1] << 8)
                         | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
        uint32_t word_hi = (uint32_t)buf[4]       | ((uint32_t)buf[5] << 8)
                         | ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);

        if (flash_wait_bsy() != BL_OK) {
            flash_lock();
            return BL_TIMEOUT;
        }
        FLASH_CR |= FLASH_CR_PG;

        /* Double-word program: write low word, then high word */
        *(volatile uint32_t *)(address + pos)       = word_lo;
        *(volatile uint32_t *)(address + pos + 4UL) = word_hi;

        if (flash_wait_bsy() != BL_OK) {
            FLASH_CR &= ~FLASH_CR_PG;
            flash_lock();
            return BL_TIMEOUT;
        }

        if (FLASH_SR & FLASH_SR_ERR_MASK) {
            FLASH_CR &= ~FLASH_CR_PG;
            flash_lock();
            return BL_ERROR;
        }

        FLASH_SR  = FLASH_SR_EOP;
        FLASH_CR &= ~FLASH_CR_PG;

        /* Read-back verify: confirm programmed data matches */
        uint32_t rb_lo = *(volatile uint32_t *)(address + pos);
        uint32_t rb_hi = *(volatile uint32_t *)(address + pos + 4UL);
        if (rb_lo != word_lo || rb_hi != word_hi) {
            flash_lock();
            return BL_ERROR;
        }

        pos += 8U;
    }

    flash_lock();
    return BL_OK;
}
