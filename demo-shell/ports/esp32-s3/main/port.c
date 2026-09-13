/* ESP32-S3 port of the demo shell.
 *
 * The console is UART0 through the board's USB to UART bridge at 115200, the
 * rate the pod captures at. */

#include "driver/temperature_sensor.h"
#include "driver/uart.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "shell.h"

#define UART UART_NUM_0

static uint8_t scratch[1024];
static temperature_sensor_handle_t tsens;

static void put(char c)
{
    uart_write_bytes(UART, &c, 1);
}

/* Blocks for one tick when idle, which is what yields to the idle task and
 * keeps the task watchdog quiet. */
static int get(void)
{
    uint8_t b;
    return uart_read_bytes(UART, &b, 1, 1) == 1 ? b : -1;
}

static uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static int temp_mc(void)
{
    float c = 0.0f;

    if (!tsens || temperature_sensor_get_celsius(tsens, &c) != ESP_OK)
        return -273000;
    return (int)(c * 1000.0f);
}

static void uid(char *out, size_t n)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t mac[6];
    size_t pos = 0;
    size_t i;

    if (esp_efuse_mac_get_default(mac) != ESP_OK) {
        out[0] = 0;
        return;
    }
    for (i = 0; i < sizeof mac && pos + 2 < n; i++) {
        out[pos++] = hex[mac[i] >> 4];
        out[pos++] = hex[mac[i] & 0xF];
    }
    out[pos] = 0;
}

static void reset(void)
{
    esp_restart();
}

void app_main(void)
{
    static shell_port port;
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    temperature_sensor_config_t tcfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    uint32_t flash_bytes = 0;

    uart_driver_install(UART, 256, 0, 0, NULL, 0);
    uart_param_config(UART, &cfg);

    if (temperature_sensor_install(&tcfg, &tsens) != ESP_OK ||
        temperature_sensor_enable(tsens) != ESP_OK)
        tsens = NULL;

    esp_flash_get_size(NULL, &flash_bytes);

    port.board = "esp32-s3";
    port.chip = "ESP32-S3";
    port.flash_kb = flash_bytes / 1024;
    port.ram_kb = 512;
    port.put = put;
    port.get = get;
    port.millis = millis;
    port.temp_mc = tsens ? temp_mc : 0;
    port.uid = uid;
    port.reset = reset;
    port.scratch = scratch;
    port.scratch_len = sizeof scratch;

    shell_run(&port);
}
