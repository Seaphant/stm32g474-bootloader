# UART Update Protocol

## Physical Layer

- USART2 on PA2 (TX) / PA3 (RX), AF7
- 115 200 baud, 8-N-1
- Connected to the ST-Link VCP on Nucleo-G474RE

## Packet Format

### Host → Device (Command)

```
Byte   Field        Description
─────  ───────────  ────────────────────────────────
0      SOF          0x5A
1      CMD          command identifier
2-3    LEN          payload length, little-endian
4..    DATA         0 to 256 bytes
N..N+1 CRC16        CRC16-CCITT over [CMD, LEN, DATA]
```

### Device → Host (Response)

```
Byte   Field        Description
─────  ───────────  ────────────────────────────────
0      SOF          0xA5
1      STATUS       0x00 = OK, else error code
2-3    LEN          payload length, little-endian
4..    DATA         0 to 256 bytes
N..N+1 CRC16        CRC16-CCITT over [STATUS, LEN, DATA]
```

## CRC16-CCITT

- Polynomial: 0x1021
- Initial value: 0xFFFF
- No final XOR
- Computed over all fields except SOF

## Commands

| ID   | Name    | Payload (Host)                 | Response Data    | Notes |
|------|---------|-------------------------------|------------------|-------|
| 0x01 | PING    | (none)                        | (none)           | Liveness check |
| 0x02 | VERSION | (none)                        | [major, minor, patch] | 3 bytes |
| 0x03 | ERASE   | (none)                        | (none)           | Erases full app region; ~2 s |
| 0x04 | WRITE   | [offset: 4B LE] [data: 1-248B]| (none)           | Offset relative to 0x0801 0000; 8-byte aligned |
| 0x05 | VERIFY  | [crc32: 4B LE] [size: 4B LE]  | (none)           | CRC over code after header |
| 0x06 | BOOT    | (none)                        | (none)           | Validates, then jumps |

## Status Codes

| Code | Name           | Meaning |
|------|----------------|---------|
| 0x00 | OK             | Success |
| 0x01 | ERR_CMD        | Unknown command ID |
| 0x02 | ERR_LEN        | Payload too short for command |
| 0x03 | ERR_CRC        | Packet CRC mismatch |
| 0x04 | ERR_ERASE      | Flash erase failed |
| 0x05 | ERR_WRITE      | Flash write failed |
| 0x06 | ERR_ADDR       | Address out of application range |
| 0x07 | ERR_VERIFY     | CRC verification mismatch |
| 0x08 | ERR_NO_APP     | No valid application image |

## Typical Session

```
Host                          Device
─────                         ──────
PING            ──────────►
                ◄──────────   OK

VERSION         ──────────►
                ◄──────────   OK [1, 0, 0]

ERASE           ──────────►
                ◄──────────   OK              (may take seconds)

WRITE(0, d0)    ──────────►
                ◄──────────   OK
WRITE(248, d1)  ──────────►
                ◄──────────   OK
  ...repeat...

VERIFY(crc, sz) ──────────►
                ◄──────────   OK

BOOT            ──────────►
                ◄──────────   OK
                              (device jumps to application)
```

## Error Handling

- On any NACK, the host may retry the command.
- If ERASE or WRITE fails, the device stays in bootloader mode;
  the host should retry or abort.
- The BOOT command validates the image before jumping.  If
  validation fails it returns ERR_NO_APP and stays in
  bootloader mode.
