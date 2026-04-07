# STM32G474 Custom UART Bootloader

A bare-metal bootloader for the STM32G474RE (Cortex-M4F) with UART
firmware update, CRC32 image verification, and safe boot-decision
logic.  Written in register-level C with no HAL dependency.

## Features

- **Flash partitioning** — 64 KB bootloader + 448 KB application, enforced
  by separate linker scripts
- **Image header** — 512-byte metadata block with magic number, CRC32,
  semantic version, and vector-table offset
- **CRC32 integrity check** — validates the application image before every
  boot; corrupted images stay in bootloader mode
- **UART update protocol** — binary command/response protocol over USART2
  with CRC16 packet integrity
- **Safe jump-to-app** — full peripheral teardown, VTOR relocation, MSP
  reload, DSB/ISB barriers
- **Python host tool** — `uart_updater.py` with progress bar, verification,
  and error handling
- **Zero libc dependency** — built with `-nostdlib -ffreestanding`

## Repository Structure

```
stm32g474-bootloader/
├── bootloader/           Bootloader firmware
│   ├── include/          Header files
│   ├── src/              Source files
│   ├── linker/           Linker script
│   └── Makefile
├── app/                  Demo application (LED blink + UART banner)
│   ├── include/
│   ├── src/
│   ├── linker/
│   └── Makefile
├── common/               Shared headers (memory map, image header, registers)
│   └── include/
├── tools/                Host-side Python scripts
│   ├── pack_image.py     Post-link image patcher
│   ├── uart_updater.py   UART firmware updater
│   └── requirements.txt
├── docs/                 Design documentation
│   ├── architecture.md
│   ├── protocol.md
│   └── interview_notes.md
├── openocd.cfg           OpenOCD config for Nucleo-G474RE
├── Makefile              Top-level build orchestration
└── README.md
```

## Prerequisites

- **ARM GCC toolchain** (`arm-none-eabi-gcc` 10+)
- **GNU Make**
- **Python 3.8+** with `pyserial` (`pip install -r tools/requirements.txt`)
- **OpenOCD** (optional, for direct flash programming)
- **Hardware:** Nucleo-G474RE or any STM32G474 board with USART2 on PA2/PA3

## Building

```bash
# Build everything (bootloader + application + packed image)
make

# Build individually
make bootloader
make app
make pack
```

## Flashing

### Option A: OpenOCD + ST-Link

```bash
# Flash the bootloader
make flash-bl

# Flash the packed application image
make flash-app
```

### Option B: UART Update (once bootloader is flashed)

```bash
# Hold the user button (PC13) during reset to enter bootloader mode,
# then run the updater:
python3 tools/uart_updater.py /dev/ttyACM0 app/build/application_packed.bin
```

Expected output:

```
Image : 1536 bytes total, code 1024 bytes, CRC 0xA1B2C3D4
PING  ... OK
VER   ... 1.0.0
ERASE ... OK
WRITE [####################] 100%  (1536/1536)  OK
CRC   ... OK
BOOT  ... OK

Firmware update complete.
```

## Boot Behaviour

| Condition | Behaviour |
|---|---|
| Valid app image, button not pressed | Jump to application |
| Valid app image, button held at reset | Stay in bootloader (update mode) |
| No valid image (erased or corrupt) | Stay in bootloader (update mode) |
| Update interrupted (partial write) | CRC check fails → bootloader mode on next boot |

## Memory Map

```
0x0800 0000 ┬───────────────────┐
            │   BOOTLOADER      │  64 KB
0x0801 0000 ├───────────────────┤
            │   Image Header    │  512 B
0x0801 0200 ├───────────────────┤
            │   APPLICATION     │  ~448 KB
0x0808 0000 ┴───────────────────┘
```

## Design Documentation

- [Architecture](docs/architecture.md) — memory map, boot flow, design decisions
- [Protocol Specification](docs/protocol.md) — packet format, commands, error codes
- [Interview Notes](docs/interview_notes.md) — talking points for each technical concept

## License

This project is released for educational and portfolio purposes.
