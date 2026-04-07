#!/usr/bin/env python3
"""
uart_updater.py — Host-side firmware updater for the STM32G474 bootloader.

Communicates over UART using the bootloader's binary protocol:

  Host → device packet:
    [SOF 0x5A] [CMD 1B] [LEN 2B LE] [DATA 0..256B] [CRC16 2B]

  Device → host response:
    [SOF 0xA5] [STATUS 1B] [LEN 2B LE] [DATA 0..256B] [CRC16 2B]

Typical session:
    1. PING        — verify bootloader is listening
    2. VERSION     — read bootloader version
    3. ERASE       — erase entire application region
    4. WRITE ×N    — stream the packed image in 256-byte chunks
    5. VERIFY      — ask the target to CRC-check what was written
    6. BOOT        — jump to the new application

Usage:
    python3 uart_updater.py /dev/ttyACM0 app/build/application_packed.bin
"""

import argparse
import struct
import sys
import time

import serial

# --- Protocol constants ---------------------------------------------------

SOF_CMD         = 0x5A
SOF_RESP        = 0xA5

CMD_PING        = 0x01
CMD_VERSION     = 0x02
CMD_ERASE       = 0x03
CMD_WRITE       = 0x04
CMD_VERIFY      = 0x05
CMD_BOOT        = 0x06

STATUS_OK       = 0x00

CHUNK_SIZE      = 256       # must be a multiple of 8 (flash alignment)
HEADER_SIZE     = 512


# --- CRC16-CCITT ----------------------------------------------------------

def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


# --- Packet helpers --------------------------------------------------------

def build_packet(cmd: int, data: bytes = b"") -> bytes:
    payload = struct.pack("<BH", cmd, len(data)) + data
    crc = crc16_ccitt(payload)
    return bytes([SOF_CMD]) + payload + struct.pack("<H", crc)


def recv_response(ser: serial.Serial, timeout: float = 5.0):
    ser.timeout = timeout

    sof = ser.read(1)
    if not sof or sof[0] != SOF_RESP:
        raise RuntimeError(f"Bad SOF: {sof.hex() if sof else 'timeout'}")

    raw = ser.read(3)
    if len(raw) < 3:
        raise RuntimeError("Timeout reading status/length")
    status = raw[0]
    length = struct.unpack_from("<H", raw, 1)[0]

    data = ser.read(length) if length else b""
    if len(data) < length:
        raise RuntimeError("Timeout reading data payload")

    crc_bytes = ser.read(2)
    if len(crc_bytes) < 2:
        raise RuntimeError("Timeout reading CRC")
    recv_crc = struct.unpack("<H", crc_bytes)[0]

    calc_crc = crc16_ccitt(bytes([status]) + raw[1:3] + data)
    if calc_crc != recv_crc:
        raise RuntimeError(f"CRC mismatch: calc 0x{calc_crc:04X} != recv 0x{recv_crc:04X}")

    return status, data


def send_cmd(ser, cmd, data=b"", timeout=5.0):
    ser.write(build_packet(cmd, data))
    return recv_response(ser, timeout)


# --- Main ------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="STM32G474 UART Firmware Updater")
    ap.add_argument("port", help="Serial port (e.g. /dev/ttyACM0, COM3)")
    ap.add_argument("image", help="Packed application image (.bin)")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    ap.add_argument("--no-boot", action="store_true",
                    help="Skip the BOOT command after flashing")
    args = ap.parse_args()

    with open(args.image, "rb") as f:
        image = f.read()

    if len(image) < HEADER_SIZE:
        sys.exit("Error: image file is too small")

    magic, _, img_size, img_crc = struct.unpack_from("<IIII", image, 0)
    if magic != 0x424F4F54:
        sys.exit(f"Error: bad magic 0x{magic:08X}")

    print(f"Image : {len(image)} bytes total, code {img_size} bytes, CRC 0x{img_crc:08X}")

    ser = serial.Serial(args.port, args.baud, timeout=1)
    time.sleep(0.1)
    ser.reset_input_buffer()

    # 1. Ping
    print("PING  ... ", end="", flush=True)
    st, _ = send_cmd(ser, CMD_PING)
    if st != STATUS_OK:
        sys.exit(f"FAIL (0x{st:02X})")
    print("OK")

    # 2. Version
    print("VER   ... ", end="", flush=True)
    st, d = send_cmd(ser, CMD_VERSION)
    if st == STATUS_OK and len(d) >= 3:
        print(f"{d[0]}.{d[1]}.{d[2]}")
    else:
        print("unknown")

    # 3. Erase
    print("ERASE ... ", end="", flush=True)
    st, _ = send_cmd(ser, CMD_ERASE, timeout=30.0)
    if st != STATUS_OK:
        sys.exit(f"FAIL (0x{st:02X})")
    print("OK")

    # 4. Write
    total = len(image)
    written = 0
    while written < total:
        chunk = image[written : written + CHUNK_SIZE]
        payload = struct.pack("<I", written) + chunk
        st, _ = send_cmd(ser, CMD_WRITE, payload)
        if st != STATUS_OK:
            sys.exit(f"\nWrite failed at offset 0x{written:08X} (0x{st:02X})")
        written += len(chunk)
        pct = written * 100 // total
        bar = "#" * (pct // 5) + "-" * (20 - pct // 5)
        print(f"\rWRITE [{bar}] {pct:3d}%  ({written}/{total})", end="", flush=True)
    print("  OK")

    # 5. Verify
    print("CRC   ... ", end="", flush=True)
    st, _ = send_cmd(ser, CMD_VERIFY, struct.pack("<II", img_crc, img_size))
    if st != STATUS_OK:
        sys.exit(f"FAIL (0x{st:02X})")
    print("OK")

    # 6. Boot
    if not args.no_boot:
        print("BOOT  ... ", end="", flush=True)
        st, _ = send_cmd(ser, CMD_BOOT)
        if st != STATUS_OK:
            sys.exit(f"FAIL (0x{st:02X})")
        print("OK")

    print("\nFirmware update complete.")
    ser.close()


if __name__ == "__main__":
    main()
