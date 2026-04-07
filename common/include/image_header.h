#ifndef IMAGE_HEADER_H
#define IMAGE_HEADER_H

#include <stdint.h>

#define IMAGE_MAGIC             0x424F4F54UL    /* "BOOT" little-endian */
#define IMAGE_HEADER_VERSION    0x0001U
#define IMAGE_HEADER_SIZE       512U

/*
 * Application image header — sits at the start of the app flash region.
 * CRC32 and image_size are patched by pack_image.py after linking.
 */
typedef struct {
    uint32_t magic;                 /* Must equal IMAGE_MAGIC                   */
    uint32_t header_version;        /* Header format version                    */
    uint32_t image_size;            /* Byte count of code after this header     */
    uint32_t crc32;                 /* CRC32 over the code bytes                */
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t vector_table_offset;   /* Offset from header start to vector table */
    uint8_t  _reserved[IMAGE_HEADER_SIZE - 32U];
} __attribute__((packed)) image_header_t;

_Static_assert(sizeof(image_header_t) == IMAGE_HEADER_SIZE,
               "image_header_t must be exactly 512 bytes");

#endif /* IMAGE_HEADER_H */
