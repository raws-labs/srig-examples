# demo-shell

An interactive serial shell that runs on every SiliconRig board type. It boots,
identifies the chip it is running on, self-tests, and then answers typed
commands. One portable core in `src/` plus a small port per chip.

```
SiliconRig demo shell v1 | board=stm32-h753 | chip=STM32H753ZI
uid=3839333133335117003E0019 flash=2048K ram=1024K
selftest: uart ok
selftest: timer ok
selftest: uid ok
selftest: ram ok
selftest: flash-size ok
selftest: temp-sensor ok
selftest: 6 passed, 0 failed
Type 'help'. Heartbeat stops on first keypress.
[hb] stm32-h753 up=5s t=31.3C
> id
chip=STM32H753ZI uid=3839333133335117003E0019 flash=2048K ram=1024K
> bench
bench: 1000000 iters in 406 ms -> 2463000 iters/s
```

## Commands

`help`, `id`, `uptime`, `temp`, `echo <text>`, `bench`, `selftest`, `reset`.

`selftest` ends with `##srig-exit:0##`, which `srig run` turns into the process
exit code, so the shell doubles as a CI smoke test for a board:

```bash
srig run demo-shell-stm32-h753.bin --board stm32-h753 --expect "0 failed"
```

The banner and self-test only appear once, at boot. A viewer who attaches later
sees the heartbeat line instead, which repeats every 5 seconds while nobody is
typing and comes back a minute after the last keystroke. Opening a serial port
glitches the line and the board reads a byte nobody sent, so only real
characters count as input: otherwise attaching to a board would silence it.

## Build

Each port writes its artifact into its own `build/`.

```bash
cd ports/stm32 && make                 # both Nucleos, needs arm-none-eabi-gcc
cd ports/rp2350 && PICO_SDK_PATH=... cmake -B build -DPICO_BOARD=pico2_w && cmake --build build
cd ports/esp32-s3 && idf.py set-target esp32s3 && idf.py build && idf.py merge-bin
cd ports/host && make                  # runs on the machine you build it on
```

| Port | Artifact | Flashed as |
|---|---|---|
| `ports/stm32` | `build/demo-shell-stm32-h753.bin`, `build/demo-shell-stm32-f446.bin` | raw image at `0x08000000` |
| `ports/rp2350` | `build/demo-shell-rp2350.uf2` | `.uf2` |
| `ports/esp32-s3` | `build/merged-binary.bin` | merged image at `0x0` |

The host port is a development aid: it runs the same `src/shell.c` on a
terminal, so the portable half can be exercised without a board.

## Adding a port

Fill in a `shell_port` from `src/shell.h` and call `shell_run()`. The required
functions are `put`, `get`, `millis`, `uid` and `reset`, plus a scratch buffer
for the RAM check. `temp_mc` is optional: leave it NULL and the `temp` command
disappears from `help`.

Ports run from the reset default clock with no PLL, so there is no clock tree
to configure and the UART divisor is a compile time constant. That also means
`bench` measures the chip at its reset clock, 64 MHz on the H753 rather than
its 480 MHz maximum, so the number compares ports and not silicon.
