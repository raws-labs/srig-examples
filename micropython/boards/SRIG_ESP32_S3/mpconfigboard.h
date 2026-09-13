#define MICROPY_HW_BOARD_NAME               "SiliconRig ESP32-S3"
#define MICROPY_HW_MCU_NAME                 "ESP32-S3"

// The console is the board's USB to UART bridge, not native USB.
#define MICROPY_HW_ENABLE_UART_REPL         (1)

#define MICROPY_HW_I2C0_SCL                 (9)
#define MICROPY_HW_I2C0_SDA                 (8)

// The I2C target role links the new ESP-IDF i2c driver while machine.I2C still
// uses the legacy one, and IDF 5.4 aborts during global constructors when both
// are linked: "CONFLICT! driver_ng is not allowed to be used with this old
// driver". The master role is what matters here, so drop the target role rather
// than pin a different IDF.
#define MICROPY_PY_MACHINE_I2C_TARGET       (0)
