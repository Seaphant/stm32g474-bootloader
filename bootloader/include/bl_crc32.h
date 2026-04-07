#ifndef BL_CRC32_H
#define BL_CRC32_H

#include <stdint.h>

uint32_t crc32_compute(const uint8_t *data, uint32_t len);

#endif /* BL_CRC32_H */
