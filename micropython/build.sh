#!/usr/bin/env bash
# Builds MicroPython for every SiliconRig board type and collects the images
# under build/ with the names the release uses.
#
#   ./build.sh            all four boards
#   ./build.sh stm32      just the two Nucleos
#   ./build.sh esp32      needs a sourced ESP-IDF
#   ./build.sh rp2
set -euo pipefail
cd "$(dirname "$0")"

VERSION=v1.29.0
SRC=.src
OUT=build
BOARDS=$PWD/boards

log() { printf '\n== %s\n' "$1"; }

if [ ! -d "$SRC" ]; then
    log "cloning MicroPython $VERSION"
    git clone --depth 1 --branch "$VERSION" https://github.com/micropython/micropython.git "$SRC"
fi

mkdir -p "$OUT"
log "mpy-cross"
make -C "$SRC/mpy-cross" -j"$(nproc)"

build_stm32() {
    for board in NUCLEO_F446RE NUCLEO_H753ZI; do
        log "stm32 $board"
        make -C "$SRC/ports/stm32" BOARD=$board submodules
        make -C "$SRC/ports/stm32" BOARD=$board -j"$(nproc)"
    done
    # The stm32 build lays text out in two segments, so ship the hex: the
    # coordinator converts Intel HEX to a flashable image server side.
    cp "$SRC/ports/stm32/build-NUCLEO_F446RE/firmware.hex" "$OUT/micropython-stm32-f446.hex"
    cp "$SRC/ports/stm32/build-NUCLEO_H753ZI/firmware.hex" "$OUT/micropython-stm32-h753.hex"
}

build_rp2() {
    log "rp2 SRIG_RPI_PICO2_W"
    make -C "$SRC/ports/rp2" BOARD=RPI_PICO2_W submodules
    # Not the stock RPI_PICO2_W: see boards/SRIG_RPI_PICO2_W for why.
    make -C "$SRC/ports/rp2" BOARD_DIR="$BOARDS/SRIG_RPI_PICO2_W" -j"$(nproc)"
    cp "$SRC/ports/rp2/build-SRIG_RPI_PICO2_W/firmware.uf2" "$OUT/micropython-rp2350.uf2"
}

build_esp32() {
    : "${IDF_PATH:?source an ESP-IDF export.sh first}"
    log "esp32 SRIG_ESP32_S3"
    # Not the stock ESP32_GENERIC_S3: see boards/SRIG_ESP32_S3 for why.
    make -C "$SRC/ports/esp32" BOARD_DIR="$BOARDS/SRIG_ESP32_S3" submodules
    make -C "$SRC/ports/esp32" BOARD_DIR="$BOARDS/SRIG_ESP32_S3" -j"$(nproc)"
    cp "$SRC/ports/esp32/build-SRIG_ESP32_S3/firmware.bin" "$OUT/micropython-esp32-s3.bin"
}

case "${1:-all}" in
    stm32) build_stm32 ;;
    rp2)   build_rp2 ;;
    esp32) build_esp32 ;;
    all)   build_stm32; build_rp2; build_esp32 ;;
    *)     echo "usage: $0 [all|stm32|rp2|esp32]"; exit 2 ;;
esac

log "images"
ls -l "$OUT"
