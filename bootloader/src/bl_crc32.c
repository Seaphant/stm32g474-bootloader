/*
 * Software CRC32 — standard Ethernet / zlib polynomial (0xEDB88320).
 *
 * Uses the bit-by-bit reflected algorithm: no lookup table, zero RAM
 * overhead, ~0.5 s for 512 KB at 16 MHz.  Adequate for a bootloader;
 * a 1 KB lookup table would cut that to ~50 ms (see interview notes).
 */

#include "bl_crc32.h"

#define CRC32_POLY  0xEDB88320UL

uint32_t crc32_compute(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1U) ? ((crc >> 1) ^ CRC32_POLY) : (crc >> 1);
    }

    return crc ^ 0xFFFFFFFFUL;
}
