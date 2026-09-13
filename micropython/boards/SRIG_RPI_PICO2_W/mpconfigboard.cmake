# The stock Pico 2 W configuration. Kept as a copy rather than an include so the
# board is self contained; MicroPython is pinned, so it cannot drift silently.
set(PICO_BOARD "pico2_w")

set(MICROPY_PY_LWIP ON)
set(MICROPY_PY_NETWORK_CYW43 ON)

set(MICROPY_PY_BLUETOOTH ON)
set(MICROPY_BLUETOOTH_BTSTACK ON)
set(MICROPY_PY_BLUETOOTH_CYW43 ON)

set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/boards/RPI_PICO2_W/manifest.py)

if(NOT DEFINED MICROPY_HW_FLASH_STORAGE_BYTES)
    set(MICROPY_HW_FLASH_STORAGE_BYTES 2621440)
endif()
