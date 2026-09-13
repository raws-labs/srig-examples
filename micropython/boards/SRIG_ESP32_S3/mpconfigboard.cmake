# The generic S3 board pulls in sdkconfig.spiram_quad, and a board without PSRAM
# aborts in a boot loop on "PSRAM ID read error". Everything else is the stock
# generic configuration.
set(IDF_TARGET esp32s3)

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.ble
)

list(APPEND SDKCONFIG_DEFAULTS
    boards/sdkconfig.flash_qio_80m
    boards/sdkconfig.csi
)
