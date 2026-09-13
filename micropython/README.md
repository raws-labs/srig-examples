# micropython

MicroPython built for every SiliconRig board type, so a session can start on a
REPL instead of a blank board. Flash one and type Python at real silicon:

```
MicroPython v1.29.0 on 2026-09-13; NUCLEO_H753ZI with STM32H753
Type "help()" for more information.
>>> import sys; print(sys.implementation)
(name='micropython', version=(1, 29, 0, ''), _machine='NUCLEO_H753ZI with STM32H753', ...)
>>> print(sum(range(100)))
4950
```

## Build

```bash
./build.sh              # all four, esp32 needs a sourced ESP-IDF
./build.sh stm32        # arm-none-eabi-gcc only
```

The script clones MicroPython at a pinned tag into `.src`, builds each port, and
collects the images into `build/` under the names the release uses.

| board type | image | upstream board |
|---|---|---|
| `stm32-h753` | `micropython-stm32-h753.hex` | `NUCLEO_H753ZI` |
| `stm32-f446` | `micropython-stm32-f446.hex` | `NUCLEO_F446RE` |
| `esp32-s3` | `micropython-esp32-s3.bin` | `SRIG_ESP32_S3`, see below |
| `rp2350` | `micropython-rp2350.uf2` | `SRIG_RPI_PICO2_W`, see below |

## Why the STM32 boards work unmodified

Both Nucleos put the MicroPython REPL on the UART wired to the onboard ST-LINK
virtual COM port at 115200, USART3 on the H753 and USART2 on the F446. That is
the port the rig reads, so the REPL is reachable with no changes. The stm32
build lays its text out in two segments, so ship the `.hex` and let the API
convert it.

## Why the ESP32-S3 needs its own board

`boards/SRIG_ESP32_S3` is the stock generic S3 configuration with two changes,
both of which are boot failures rather than preferences:

- No PSRAM. The generic board pulls in `sdkconfig.spiram_quad`, and a module
  without PSRAM aborts on `PSRAM ID read error` and reboots forever.
- No I2C target role. It links the new ESP-IDF i2c driver while `machine.I2C`
  still uses the legacy one, and ESP-IDF 5.4 aborts during global constructors
  with `CONFLICT! driver_ng is not allowed to be used with this old driver`.
  Dropping the target role keeps the master role, which is the useful one.

## Why the Pico needs its own board

`boards/SRIG_RPI_PICO2_W` is the stock Pico 2 W configuration plus one line,
`MICROPY_HW_USB_CDC_1200BPS_TOUCH`, and it is the difference between a board
that can be handed back and one that cannot.

The wipe at the end of a session runs `picotool erase -f`, which forces the
running firmware into BOOTSEL through a pico-sdk mechanism. MicroPython does not
implement it, so the erase fails, and after three failures the board leaves the
pool with no remote way back. MicroPython does implement the other convention:
open its USB console at 1200 baud, close it, and it reboots into BOOTSEL.
Upstream enables that only on Arduino boards, so our build turns it on.

## Licence

MicroPython is MIT licensed. These are unmodified upstream sources built with
the board configuration above; see `.src/LICENSE` after a build.
