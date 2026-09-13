idf_component_register(
    SRCS "port.c" "${CMAKE_CURRENT_LIST_DIR}/../../../src/shell.c"
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../src"
    REQUIRES driver esp_timer spi_flash)
