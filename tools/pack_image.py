#!/usr/bin/env python3
"""
pack_image.py — Patch an application binary with a valid image header.

The linker produces a binary that already contains an image_header_t at
offset 0, but with placeholder values for image_size and crc32.  This
script computes the correct values and patches them in-place so the
bootloader can validate the image on the target.

Usage:
    python3 pack_image.py <input.bin> <output.bin>
"""

import struct
import sys
import zlib

HEADER_SIZE         = 512
MAGIC               = 0x424F4F54          # "BOOT" little-endian
FIELD_IMAGE_SIZE    = 8                   # offset of image_size in header
FIELD_CRC32         = 12                  # offset of crc32 in header


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <output.bin>")
        sys.exit(1)

    in_path, out_path = sys.argv[1], sys.argv[2]

    with open(in_path, "rb") as f:
        data = bytearray(f.read())

    if len(data) < HEADER_SIZE:
        sys.exit(f"Error: binary is only {len(data)} bytes (need >= {HEADER_SIZE})")

    magic = struct.unpack_from("<I", data, 0)[0]
    if magic != MAGIC:
        sys.exit(f"Error: bad magic 0x{magic:08X} (expected 0x{MAGIC:08X})")

    code = data[HEADER_SIZE:]
    image_size = len(code)
    crc = zlib.crc32(bytes(code)) & 0xFFFFFFFF

    struct.pack_into("<I", data, FIELD_IMAGE_SIZE, image_size)
    struct.pack_into("<I", data, FIELD_CRC32, crc)

    with open(out_path, "wb") as f:
        f.write(data)

    ver_major = struct.unpack_from("<I", data, 16)[0]
    ver_minor = struct.unpack_from("<I", data, 20)[0]
    ver_patch = struct.unpack_from("<I", data, 24)[0]

    print(f"Packed: {out_path}")
    print(f"  Version:    {ver_major}.{ver_minor}.{ver_patch}")
    print(f"  Total size: {len(data)} bytes")
    print(f"  Code size:  {image_size} bytes")
    print(f"  CRC32:      0x{crc:08X}")


if __name__ == "__main__":
    main()
