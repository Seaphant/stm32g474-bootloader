# Architecture

## Overview

This project implements a custom UART bootloader for the STM32G474RE
(Cortex-M4F, 512 KB flash, 96 KB contiguous SRAM).  The bootloader
occupies the first 64 KB of flash; the application lives in the
remaining 448 KB.

## Memory Map

```
 Address         Region             Size    Notes
 ─────────────── ────────────────── ─────── ─────────────────────────
 0x0800 0000     Bootloader         64 KB   32 pages × 2 KB
 0x0801 0000     Image header       512 B   magic, CRC32, version
 0x0801 0200     Application code   ~448 KB vector table here
 0x0808 0000     (end of flash)
 0x2000 0000     SRAM1 + SRAM2      96 KB   shared between BL & app
```

## Flash Partitioning

The STM32G4 flash is organised in 2 KB pages (single-bank mode).
Pages 0-31 belong to the bootloader; pages 32-255 belong to the
application.  The flash driver refuses writes below page 32, providing
a software guard against self-overwrite.

For hardware-level protection, set the WRP option bytes via ST-Link
Utility to cover pages 0-31.

## Boot Flow

```
RESET
  │
  ├─ Bootloader vector table @ 0x0800 0000
  │   └─ Reset_Handler: init .data/.bss → system_init() → main()
  │
  ├─ Check PC13 (boot pin)
  │   └─ Held low → enter update mode
  │
  ├─ Validate application image
  │   ├─ Magic == 0x424F4F54 ?
  │   ├─ Header version, size bounds, VT offset OK ?
  │   ├─ CRC32 matches stored value ?
  │   ├─ MSP points into SRAM ?
  │   └─ Reset vector is Thumb & within app flash ?
  │
  ├─ Valid → jump to application
  │   ├─ Disable IRQs
  │   ├─ Clear NVIC enables & pending
  │   ├─ Set VTOR → 0x0801 0200
  │   ├─ DSB + ISB memory barriers
  │   ├─ Load MSP from app vector table[0]
  │   └─ Branch to app vector table[1] (Reset_Handler)
  │
  └─ Invalid → stay in bootloader, run protocol_run()
```

## Image Header

The 512-byte `image_header_t` struct is embedded at offset 0 of the
application binary by the linker (`.image_header` section).  After
linking, `pack_image.py` patches the `image_size` and `crc32` fields.

| Offset | Size | Field                |
|--------|------|----------------------|
| 0x00   | 4    | magic (0x424F4F54)   |
| 0x04   | 4    | header_version       |
| 0x08   | 4    | image_size           |
| 0x0C   | 4    | crc32                |
| 0x10   | 4    | version_major        |
| 0x14   | 4    | version_minor        |
| 0x18   | 4    | version_patch        |
| 0x1C   | 4    | vector_table_offset  |
| 0x20   | 480  | reserved (0x00)      |

## Clock Configuration

The bootloader runs on the 16 MHz HSI oscillator with 0 flash
wait-states.  This avoids any dependency on an external crystal and
makes baud-rate calculation trivial (BRR = 16 000 000 / 115 200 ≈ 139).

## Design Decisions

| Decision | Rationale |
|---|---|
| Register-level drivers | Demonstrates hardware understanding; no HAL dependency |
| Software CRC32 (no table) | Zero RAM overhead; ~0.5 s for 512 KB at 16 MHz |
| C-based startup | Modern practice; vector table as const array |
| Separate linker scripts | Enforces memory isolation at build time |
| Flash bounds checking | Software guard; bootloader cannot overwrite itself |
| SysTick for UART timeout | Accurate timing without a dedicated timer peripheral |
| `-nostdlib -ffreestanding` | No libc dependency; minimal binary size |
