// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "SiliconRig Pico 2 W"

// Enable networking.
#define MICROPY_PY_NETWORK 1
#define MICROPY_PY_NETWORK_HOSTNAME_DEFAULT     "Pico2W"

// CYW43 driver configuration.
#define CYW43_USE_SPI (1)
#define CYW43_LWIP (1)
#define CYW43_GPIO (1)
#define CYW43_SPI_PIO (1)

// For debugging mbedtls - also set
// Debug level (0-4) 1=warning, 2=info, 3=debug, 4=verbose
// #define MODUSSL_MBEDTLS_DEBUG_LEVEL 1

#define MICROPY_HW_PIN_EXT_COUNT    CYW43_WL_GPIO_COUNT

int mp_hal_is_pin_reserved(int n);
#define MICROPY_HW_PIN_RESERVED(i) mp_hal_is_pin_reserved(i)

// Opening the USB console at 1200 baud and closing it again puts the board into
// BOOTSEL. Without this the session-end wipe has no way back in: picotool's
// force is a pico-sdk mechanism that MicroPython does not implement, so the
// erase fails and the board drops out of the pool until someone reaches the
// rack. Arduino images use the same convention.
#define MICROPY_HW_USB_CDC_1200BPS_TOUCH        (1)
