/* RP2350 port of the demo shell, for a Raspberry Pi Pico 2 W.
 *
 * The board's own USB port is the console: it re-enumerates after every flash,
 * which is why the shell keeps a heartbeat running until someone types. */

#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "shell.h"

#define TEMP_ADC_INPUT 4u

static uint8_t scratch[1024];

static void put(char c)
{
    putchar_raw(c); /* raw: the shell emits its own CR LF */
}

static int get(void)
{
    int c = getchar_timeout_us(0);
    return c == PICO_ERROR_TIMEOUT ? -1 : c;
}

static uint32_t millis(void)
{
    return to_ms_since_boot(get_absolute_time());
}

/* Datasheet: T = 27 - (V - 0.706) / 0.001721, with a 12 bit result over 3.3 V. */
static int temp_mc(void)
{
    uint32_t raw;
    int32_t uv;

    adc_select_input(TEMP_ADC_INPUT);
    raw = adc_read();
    uv = (int32_t)((uint64_t)raw * 3300000u / 4096u);
    return 27000 - (uv - 706000) * 1000 / 1721;
}

static void uid(char *out, size_t n)
{
    pico_unique_board_id_t id;
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;
    size_t i;

    pico_get_unique_board_id(&id);
    for (i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES && pos + 2 < n; i++) {
        out[pos++] = hex[id.id[i] >> 4];
        out[pos++] = hex[id.id[i] & 0xF];
    }
    out[pos] = 0;
}

static void reset(void)
{
    watchdog_reboot(0, 0, 0);
}

int main(void)
{
    static shell_port port;

    stdio_init_all();

    /* This board's USB port is the console, so it re-enumerates on every flash
     * and the boot banner would be written into a void. Give the host a few
     * seconds to attach, then start regardless: the heartbeat covers a viewer
     * who joins later. */
    for (int i = 0; i < 500 && !stdio_usb_connected(); i++)
        sleep_ms(10);

    adc_init();
    adc_set_temp_sensor_enabled(true);

    port.board = "rp2350";
    port.chip = "RP2350";
    port.flash_kb = PICO_FLASH_SIZE_BYTES / 1024;
    port.ram_kb = 520;
    port.put = put;
    port.get = get;
    port.millis = millis;
    port.temp_mc = temp_mc;
    port.uid = uid;
    port.reset = reset;
    port.scratch = scratch;
    port.scratch_len = sizeof scratch;

    shell_run(&port);
    return 0;
}
