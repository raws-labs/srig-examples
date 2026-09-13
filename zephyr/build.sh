#!/usr/bin/env bash
# Builds the Zephyr shell for the board types it can reach today and collects
# the images under build/ with the names the release uses.
#
# First run pulls a west workspace and the Zephyr SDK, which is several GB:
#   ./build.sh setup
#   ./build.sh
set -euo pipefail
cd "$(dirname "$0")"

VERSION=v4.4.2
SRC=.src
OUT=build
VENV=$PWD/.venv

log() { printf '\n== %s\n' "$1"; }

setup() {
    python3 -m venv "$VENV"
    "$VENV/bin/pip" install west
    "$VENV/bin/west" init -m https://github.com/zephyrproject-rtos/zephyr --mr "$VERSION" "$SRC"
    (cd "$SRC" && "$VENV/bin/west" update --narrow -o=--depth=1)
    (cd "$SRC" && "$VENV/bin/west" packages pip --install)
    # Both toolchains in one invocation installs only the last one.
    (cd "$SRC" && "$VENV/bin/west" sdk install -t arm-zephyr-eabi)
    (cd "$SRC" && "$VENV/bin/west" sdk install -t xtensa-espressif_esp32s3_zephyr-elf)
}

build() { # board, output dir, image name
    log "$1"
    # esptool lives in the venv, and the espressif build looks for it on PATH.
    (cd "$SRC" && PATH="$VENV/bin:$PATH" "$VENV/bin/west" build -p always -b "$1" ../app -d "../$OUT/$2")
    mkdir -p "$OUT"
    cp "$OUT/$2/zephyr/zephyr.bin" "$OUT/$3"
}

case "${1:-all}" in
    setup) setup ;;
    all)
        build nucleo_h753zi h753 zephyr-stm32-h753.bin
        build nucleo_f446re f446 zephyr-stm32-f446.bin
        # Zephyr puts its esp32 image at 0x0 with no separate bootloader, which
        # is exactly how the pod flashes it.
        build esp32s3_devkitc/esp32s3/procpu esp32 zephyr-esp32-s3.bin
        ;;
    *) echo "usage: $0 [setup|all]"; exit 2 ;;
esac

log "images"
ls -l "$OUT"/*.bin
