# srig-examples

Example firmware projects with hardware-in-the-loop tests that run on SiliconRig through the `siliconrig` Python SDK. ESP-IDF C firmware for esp32-s3 plus pytest. Public, github.com/raws-labs/srig-examples.

## Build, test, run
- Firmware needs a sourced ESP-IDF (`IDF_PATH` set; `idf.py` and `esptool.py` on PATH).
- `esp-now-demo/build.sh`: `idf.py set-target esp32s3` (first time) and `idf.py build` for `sender/` and `receiver/`, then `esptool.py merge_bin` into `<project>/build/<project>-merged.bin` (bootloader at 0x0, partition table at 0x8000, app at 0x10000, 16 MB, DIO). The tests flash these merged images, not the plain app `.bin`.
- `pip install siliconrig pytest`, `export SRIG_API_KEY=key_...`, then `pytest esp-now-demo/test/ -v`. Fixtures are module-scoped `Board(...)` objects that flash once per run.
- `python esp-now-demo/demo.py`: the same flow as a narrated terminal demo (used for recordings).

## Layout
- `esp-now-demo/`: two esp32-s3 boards in one test. `sender/` broadcasts over ESP-NOW and prints `TX: seq=N`; `receiver/` prints `RX: seq=N rssi=-XX` and periodic `SUMMARY:` lines; `test/test_esp_now.py` asserts reception, RSSI in -100..0 dBm, and loss under 50 percent.
- `ota-rollback-demo/` (untracked, see Open): `v1/` embeds `v2_broken.bin` (a build of `v2/`, which aborts right after boot) via `EMBED_FILES`; the serial command `stage_bad_update` writes it to the other OTA slot and reboots; the bootloader rolls back (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`, custom `partitions.csv` with `ota_0`/`ota_1`, 4 MB flash) and v1 reports `ROLLBACK DETECTED`. `test/test_ota_rollback.py` also power-cycles with `board.reset()`.

## Conventions
- Firmware prints one stable, grep-able line per event (`TX: seq=`, `partition=ota_0`, `STAGED bad update`) and tests `expect()` those strings. v1 emits its state as a periodic heartbeat rather than a one-shot banner so a test cannot miss it during the serial reconnect after a flash.
- `.gitignore` drops ESP-IDF outputs (`build/`, `sdkconfig`, `managed_components/`) and demo media (`*.gif`, `*.tape`, `*.mp4`); commit sources and `sdkconfig.defaults` only.

## Gotchas
- The ESP-NOW test holds two sessions at once, so the API key's plan must allow two concurrent sessions and two esp32-s3 boards must be free.
- After `stage_bad_update`, v2 boots far enough to print `v2.0 ALIVE` and then panics; allow up to 60 s for `ROLLBACK DETECTED`.
- The serial console is captured at 115200 baud; firmware logging at another rate shows up as garbage.

## Open
- `ota-rollback-demo/` exists only in the working tree, never committed, and has no build script for `v1/build/v1-merged.bin` (verify; presumably `esp-now-demo/build.sh` with a 4 MB flash size).
