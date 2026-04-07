#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

/*
 * STM32G474RE Memory Map
 *
 * Flash: 512KB total, 2KB pages (single-bank mode)
 * SRAM:  96KB contiguous (SRAM1 80KB + SRAM2 16KB)
 *        CCM SRAM 32KB at 0x10000000 (not used here)
 */

#define FLASH_BASE_ADDR         0x08000000UL
#define FLASH_TOTAL_SIZE        (512UL * 1024UL)
#define FLASH_PAGE_SIZE         2048UL

#define SRAM_BASE_ADDR          0x20000000UL
#define SRAM_SIZE               (96UL * 1024UL)

/* --- Bootloader region: 0x08000000 – 0x0800FFFF (64KB) --- */
#define BL_FLASH_START          FLASH_BASE_ADDR
#define BL_FLASH_SIZE           (64UL * 1024UL)
#define BL_FLASH_END            (BL_FLASH_START + BL_FLASH_SIZE)
#define BL_PAGE_COUNT           (BL_FLASH_SIZE / FLASH_PAGE_SIZE)

/* --- Application region: 0x08010000 – 0x0807FFFF (448KB) --- */
#define APP_FLASH_START         BL_FLASH_END
#define APP_FLASH_SIZE          (FLASH_TOTAL_SIZE - BL_FLASH_SIZE)
#define APP_FLASH_END           (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE)
#define APP_PAGE_START          BL_PAGE_COUNT
#define APP_PAGE_COUNT          (APP_FLASH_SIZE / FLASH_PAGE_SIZE)

/* --- Application image layout --- */
#define APP_HEADER_ADDR         APP_FLASH_START
#define APP_VECTOR_ADDR         (APP_FLASH_START + 0x200UL)

#endif /* MEMORY_MAP_H */
