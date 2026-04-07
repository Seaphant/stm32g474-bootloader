/*
 * Linker script — Application
 * STM32G474RE  (Cortex-M4)
 *
 * The first 512 bytes (FLASH_HDR) hold the image_header_t struct.
 * The vector table begins at 0x0801 0200, which satisfies the
 * Cortex-M4 VTOR alignment requirement (≥ 512 bytes for this device).
 */

MEMORY
{
    FLASH_HDR (r)   : ORIGIN = 0x08010000, LENGTH = 512
    FLASH     (rx)  : ORIGIN = 0x08010200, LENGTH = 448K - 512
    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

_estack = ORIGIN(SRAM) + LENGTH(SRAM);

ENTRY(Reset_Handler)

SECTIONS
{
    /* Image header — patched post-link by pack_image.py */
    .image_header :
    {
        KEEP(*(.image_header))
    } > FLASH_HDR

    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }

    ASSERT(. <= ORIGIN(FLASH) + LENGTH(FLASH), "Application exceeds flash region")
}
