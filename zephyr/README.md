# zephyr

A Zephyr shell for SiliconRig boards, so a session can start on something
interactive. Flash one and the board answers:

```
srig:~$ kernel version
Zephyr version 4.4.2
srig:~$ hwinfo devid
Length: 12
ID: 0x3839333133335117003e0019
srig:~$ device list
- serial@40004400 (READY)
  DT node labels: usart2
```

`hwinfo devid` returns the same chip id the demo shell prints, read off the
silicon rather than compiled in.

## Build

```bash
./build.sh setup     # west workspace plus the Zephyr SDK, several GB, once
./build.sh
```

| board type | image | Zephyr board |
|---|---|---|
| `stm32-h753` | `zephyr-stm32-h753.bin` | `nucleo_h753zi` |
| `stm32-f446` | `zephyr-stm32-f446.bin` | `nucleo_f446re` |
| `esp32-s3` | `zephyr-esp32-s3.bin` | `esp32s3_devkitc/esp32s3/procpu` |

The app is `app/`: `prj.conf` turns on the shell and the kernel, device and
hwinfo commands, and `main` is empty because the shell runs on its own.

## Why there is no rp2350 image yet

Two things stand between Zephyr and the Pico, and the second one is a hazard:

- Its console would have to be USB CDC, since Zephyr defaults to UART0 on pins
  nothing is wired to. The `cdc-acm-console` snippet covers that.
- Wiping the board at the end of a session needs a way back into BOOTSEL.
  `picotool erase -f` speaks a pico-sdk mechanism Zephyr does not implement, and
  the 1200 baud touch only works if the firmware acts on it. Zephyr can get
  there, `CONFIG_RPI_PICO_ROM_BOOTLOADER` plus the `rp2-boot-mode-retention`
  snippet plus a watcher on the CDC line rate, but until that is built and
  proven, flashing a Zephyr image to the Pico would leave a board that only
  physical access can recover.
