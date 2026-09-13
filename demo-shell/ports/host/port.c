/* Host port: runs the same shell on a terminal so the portable half can be
 * tried and tested without hardware. Not shipped to any board. */

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../../src/shell.h"

static uint8_t scratch[1024];
static struct termios saved_tio;
static int raw_active;

static void put(char c)
{
    fputc(c, stdout);
    fflush(stdout);
}

static int get(void)
{
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    unsigned char c;

    if (poll(&p, 1, 1) <= 0)
        return -1;
    if (read(STDIN_FILENO, &c, 1) != 1)
        exit(0); /* stdin closed: a piped script has finished */
    return c;
}

static uint32_t started_ms;

static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + (unsigned)ts.tv_nsec / 1000000u);
}

/* Zero based like a board's tick counter, so uptime means the same thing. */
static uint32_t millis(void)
{
    return now_ms() - started_ms;
}

static void uid(char *out, size_t n)
{
    snprintf(out, n, "484F535400000000DEADBEEF");
}

static void reset(void)
{
    if (raw_active)
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_tio);
    exit(0);
}

int main(void)
{
    static shell_port port;
    struct termios tio;

    started_ms = now_ms();

    /* Character at a time input, so typing feels like the real serial console.
     * Skipped when stdin is a pipe, which is how the tests drive it. */
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &tio) == 0) {
        saved_tio = tio;
        tio.c_lflag &= ~(unsigned)(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        raw_active = 1;
    }

    port.board = "host";
    port.chip = "posix";
    port.flash_kb = 0;
    port.ram_kb = 0;
    port.put = put;
    port.get = get;
    port.millis = millis;
    port.temp_mc = 0;
    port.uid = uid;
    port.reset = reset;
    port.scratch = scratch;
    port.scratch_len = sizeof scratch;

    shell_run(&port);
    return 0;
}
