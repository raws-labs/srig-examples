# RP2350 build of the demo shell. Needs PICO_SDK_PATH.
#   cmake -B build -DPICO_BOARD=pico2_w && cmake --build build
# Artifact: build/demo-shell-rp2350.uf2
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(demo_shell C CXX ASM)
set(CMAKE_C_STANDARD 11) # the SDK headers use static_assert
pico_sdk_init()

add_executable(demo-shell-rp2350 port.c ../../src/shell.c)
target_include_directories(demo-shell-rp2350 PRIVATE ../../src)
target_link_libraries(demo-shell-rp2350 pico_stdlib pico_unique_id hardware_adc hardware_watchdog)

# The board's USB port is the serial console, so stdio goes there and not to a
# UART nothing is wired to.
pico_enable_stdio_usb(demo-shell-rp2350 1)
pico_enable_stdio_uart(demo-shell-rp2350 0)
pico_add_extra_outputs(demo-shell-rp2350)
