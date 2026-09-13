# srig-examples

Example firmware projects that run on SiliconRig, with hardware-in-the-loop tests through the `siliconrig` Python SDK. C firmware for all four board types plus pytest. Public, github.com/raws-labs/srig-examples.

## Build, test, run
- `demo-shell/` builds per port and needs only that port's toolchain: `ports/stm32` needs `arm-none-eabi-gcc` and nothing else (`make` builds both Nucleos), `ports/rp2350` needs `PICO_SDK_PATH` (`cmake -B build -DPICO_BOARD=pico2_w && cmake --build build`), `ports/esp32-s3` needs a sourced ESP-IDF (`idf.py set-target esp32s3 && idf.py build && idf.py merge-bin`, the artifact is `build/merged-binary.bin`), `ports/host` needs a host compiler. Verified on the live rig on all four board types.
- ESP-IDF firmware needs a sourced ESP-IDF (`IDF_PATH` set; `idf.py` and `esptool.py` on PATH).
- `esp-now-demo/build.sh`: `idf.py set-target esp32s3` (first time) and `idf.py build` for `sender/` and `receiver/`, then `esptool.py merge_bin` into `<project>/build/<project>-merged.bin` (bootloader at 0x0, partition table at 0x8000, app at 0x10000, 16 MB, DIO). The tests flash these merged images, not the plain app `.bin`.
- `pip install siliconrig pytest`, `export SRIG_API_KEY=key_...`, then `pytest esp-now-demo/test/ -v`. Fixtures are module-scoped `Board(...)` objects that flash once per run.
- `python esp-now-demo/demo.py`: the same flow as a narrated terminal demo (used for recordings).

## Layout
- `demo-shell/`: an interactive serial shell for every board type. `src/shell.c` is the whole user facing behavior and is shared by all ports; `src/shell.h` is the port interface, roughly `put`, `get`, `millis`, `uid`, `reset`, an optional `temp_mc`, and a scratch buffer for the RAM check. `ports/<chip>/` fills that in. The shell prints a banner and a self-test at boot, repeats a heartbeat line every 5 s while nobody is typing and again a minute after the last keystroke, and answers `help`, `id`, `uptime`, `temp`, `echo`, `bench`, `selftest`, `reset`. `selftest` ends with `##srig-exit:0##`, which `srig run` turns into an exit code.
- `esp-now-demo/`: two esp32-s3 boards in one test. `sender/` broadcasts over ESP-NOW and prints `TX: seq=N`; `receiver/` prints `RX: seq=N rssi=-XX` and periodic `SUMMARY:` lines; `test/test_esp_now.py` asserts reception, RSSI in -100..0 dBm, and loss under 50 percent.
- `ota-rollback-demo/` (untracked, see Open): `v1/` embeds `v2_broken.bin` (a build of `v2/`, which aborts right after boot) via `EMBED_FILES`; the serial command `stage_bad_update` writes it to the other OTA slot and reboots; the bootloader rolls back (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`, custom `partitions.csv` with `ota_0`/`ota_1`, 4 MB flash) and v1 reports `ROLLBACK DETECTED`. `test/test_ota_rollback.py` also power-cycles with `board.reset()`.

## Conventions
- Firmware prints one stable, grep-able line per event (`TX: seq=`, `partition=ota_0`, `STAGED bad update`) and tests `expect()` those strings. v1 emits its state as a periodic heartbeat rather than a one-shot banner so a test cannot miss it during the serial reconnect after a flash.
- `.gitignore` drops ESP-IDF outputs (`build/`, `sdkconfig`, `managed_components/`) and demo media (`*.gif`, `*.tape`, `*.mp4`); commit sources and `sdkconfig.defaults` only.

## Gotchas
- Opening a serial port glitches the line and the board reads a byte nobody sent, seen on the rig as a `00 ff` pair. `demo-shell` therefore counts only CR, LF, backspace and printable ASCII as input; counting any byte let a single port open stop the heartbeat for good and the board looked dead for the rest of its life.
- Serial output from an esp32-s3 is unreadable for roughly the first five seconds after a flash, so a boot banner never survives on that board type. The demo shell's heartbeat is what makes it usable; do not rely on one shot boot output there.
- The rp2350's own USB port is the console, so it re-enumerates on every flash and boot output is written into a void. Its port waits up to 5 s for `stdio_usb_connected()` before printing, then starts regardless.
- The STM32 ADC common register block starts with a read only CSR, so `ADC_CCR` is at `+0x304` on the F4 and `+0x308` on the H7. Writing the sensor enable to the block base silently does nothing and the temperature reads as garbage.
- After a session on an stm32-h753 ends, the board is power cycled and takes several seconds to re-enumerate; a new session started immediately fails with "no available board".
- The ESP-NOW test holds two sessions at once, so the API key's plan must allow two concurrent sessions and two esp32-s3 boards must be free.
- After `stage_bad_update`, v2 boots far enough to print `v2.0 ALIVE` and then panics; allow up to 60 s for `ROLLBACK DETECTED`.
- The serial console is captured at 115200 baud; firmware logging at another rate shows up as garbage.

## Open
- `ota-rollback-demo/` exists only in the working tree, never committed, and has no build script for `v1/build/v1-merged.bin` (verify; presumably `esp-now-demo/build.sh` with a 4 MB flash size).
