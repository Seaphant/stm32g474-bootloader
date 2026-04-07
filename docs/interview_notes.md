# Interview Talking Points

Key technical concepts in this project and how to discuss them.

---

## 1. Memory-Mapped I/O and Volatile

**What to say:** "All peripheral registers are accessed through volatile
pointers derived from base addresses in the reference manual. The
`volatile` qualifier prevents the compiler from caching register reads
or reordering accesses — critical because hardware can change these
values at any time."

**Follow-up if asked about HAL:** "I deliberately avoided the HAL to
demonstrate register-level understanding. In production, HAL can speed
up development, but for a bootloader where binary size and
determinism matter, direct register access is standard."

---

## 2. Flash Programming Model

**What to say:** "The STM32G4 flash requires double-word (8-byte)
aligned writes. My driver assembles partial data into an 8-byte
buffer, padding unused bytes with 0xFF to leave erased cells unchanged.
Writes are bounded-checked at the driver level so the bootloader can
never overwrite itself."

**Deeper dive:** Page erase sets PER + PNB + STRT in FLASH_CR; the
BSY flag blocks until the operation completes. The unlock sequence
writes two magic keys to FLASH_KEYR.

---

## 3. CRC32 — Algorithm vs. Hardware Trade-off

**What to say:** "I used a software CRC32 with the standard Ethernet
polynomial (0xEDB88320, reflected). The bit-by-bit approach uses zero
RAM but takes ~0.5 s for 512 KB at 16 MHz. A 1 KB lookup table would
cut that to ~50 ms. The STM32G4 also has a hardware CRC unit, but it
uses a different polynomial by default and would require configuration
— the software approach is portable and easier to match on the host
side."

---

## 4. Vector Table and VTOR

**What to say:** "On Cortex-M4, the vector table is an array of 32-bit
entries: entry 0 is the initial stack pointer, entry 1 is the reset
handler, and the rest are exception/interrupt handlers. The VTOR
register lets you relocate this table. Before jumping to the app, I
set VTOR to the app's vector table address, issue DSB and ISB
barriers to ensure the pipeline sees the new table, then load MSP
and branch to the reset vector."

**Why DSB/ISB:** "DSB ensures all memory writes (including the VTOR
write) complete before we proceed. ISB flushes the instruction
pipeline so the processor fetches from the correct vector table."

---

## 5. Jump-to-Application Sequence

**What to say:** "The handoff sequence is: disable interrupts, disable
SysTick, clear all NVIC enable and pending bits, set VTOR, load MSP
from the app's vector table, re-enable interrupts, and branch to the
reset handler. Skipping any step can cause the app to crash — for
example, a stale pending interrupt would fire with the old bootloader
handler address, which no longer exists in the app's vector table."

---

## 6. Linker Scripts and Memory Partitioning

**What to say:** "I use two linker scripts with non-overlapping MEMORY
regions. The bootloader gets 0x0800_0000..0x0800_FFFF; the app gets
0x0801_0000..0x0807_FFFF. The app linker reserves the first 512 bytes
for the image header (`.image_header` output section in FLASH_HDR),
and the vector table starts at 0x0801_0200 — naturally 512-byte
aligned, which satisfies the VTOR alignment requirement."

---

## 7. Image Header and Post-Link Patching

**What to say:** "The header is a C struct placed in its own linker
section. At link time, `image_size` and `crc32` are set to 0xFFFFFFFF.
A Python script reads the linked binary, computes CRC32 over the code
bytes, and patches those two fields. This way the binary can be
flashed directly with ST-Link or sent via the UART updater — both
paths get a valid header."

---

## 8. UART Protocol Design

**What to say:** "The protocol uses a simple framed packet format with
SOF byte, command/status, length, optional data, and CRC16-CCITT.
CRC16 is computed incrementally as bytes arrive, so no extra buffer is
needed for verification. The protocol is command-response — the host
always waits for an ACK before sending the next command. This makes
error handling straightforward and avoids flow-control issues."

---

## 9. Startup Code in C

**What to say:** "I wrote the startup in C rather than assembly. The
vector table is a `const uint32_t` array in the `.isr_vector` section.
Exception handlers are declared as weak aliases to `Default_Handler`
so any module can override them. `Reset_Handler` copies `.data` from
flash to SRAM, zeroes `.bss`, then calls `main()`. This is the same
work a traditional assembly startup does, but it's more readable and
maintainable."

---

## 10. Build System and Toolchain Flags

**Key flags to know:**
- `-ffreestanding`: No hosted-environment assumptions
- `-nostdlib`: No libc linked; we supply our own runtime
- `-ffunction-sections -fdata-sections` + `--gc-sections`: Dead-code
  elimination at link time
- `-Os`: Optimize for size (standard for bootloaders)
- `-lgcc`: Provides compiler support routines (e.g. 64-bit division)
