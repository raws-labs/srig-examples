/* Portable interactive demo shell. See shell.h for the port interface. */

#include "shell.h"

#define HEARTBEAT_MS 5000u
/* Opening the serial port glitches the line and the board reads a byte nobody
 * typed, so the heartbeat comes back after this long without real input. A
 * board that has gone quiet for good is the one failure this firmware exists to
 * prevent. */
#define HEARTBEAT_RESUME_MS 60000u
#define LINE_MAX 64
#define BENCH_ITERS 1000000u

static const shell_port *P;

/* ---- output ---- */

static void out(const char *s)
{
    while (*s)
        P->put(*s++);
}

static void nl(void)
{
    P->put('\r');
    P->put('\n');
}

/* Milli units with one decimal, e.g. 34210 -> 34.2 */
static void out_milli(int mv);

static void out_u32(uint32_t v)
{
    char buf[11];
    int i = 0;
    if (v == 0) {
        P->put('0');
        return;
    }
    while (v) {
        buf[i++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (i)
        P->put(buf[--i]);
}

static void out_milli(int mv)
{
    if (mv < 0) {
        P->put('-');
        mv = -mv;
    }
    out_u32((uint32_t)mv / 1000);
    P->put('.');
    out_u32((uint32_t)(mv % 1000) / 100);
}

/* ---- input ---- */

/* A line glitch delivers NUL or a framing error byte, neither of which is
 * something a person typed. */
static int is_input_char(int c)
{
    return c == '\r' || c == '\n' || c == 0x08 || c == 0x7f ||
           (c >= ' ' && c < 0x7f);
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* Splits off the first space separated word, returns the rest. */
static char *split(char *s)
{
    while (*s && *s != ' ')
        s++;
    if (!*s)
        return s;
    *s++ = 0;
    while (*s == ' ')
        s++;
    return s;
}

/* ---- commands ---- */

static void cmd_help(void)
{
    out("id        chip id, flash and ram size");
    nl();
    out("uptime    milliseconds since boot");
    nl();
    if (P->temp_mc) {
        out("temp      on-die temperature sensor");
        nl();
    }
    out("echo <text>");
    nl();
    out("bench     integer loop, prints iterations/s");
    nl();
    out("selftest  runs every check, prints ##srig-exit:N##");
    nl();
    out("reset     software reset");
    nl();
}

static void cmd_id(void)
{
    char uid[33];
    P->uid(uid, sizeof uid);
    out("chip=");
    out(P->chip);
    out(" uid=");
    out(uid);
    out(" flash=");
    out_u32(P->flash_kb);
    out("K ram=");
    out_u32(P->ram_kb);
    out("K");
    nl();
}

static void cmd_bench(void)
{
    volatile uint32_t acc = 0;
    uint32_t t0, dt, per_s;
    uint32_t i;

    t0 = P->millis();
    for (i = 0; i < BENCH_ITERS; i++)
        acc = acc * 1664525u + 1013904223u;
    dt = P->millis() - t0;
    if (dt == 0)
        dt = 1;

    /* Scaled to avoid a 64 bit divide on a chip without one. */
    per_s = (BENCH_ITERS / dt) * 1000u;

    out("bench: ");
    out_u32(BENCH_ITERS);
    out(" iters in ");
    out_u32(dt);
    out(" ms -> ");
    out_u32(per_s);
    out(" iters/s");
    nl();
}

static int checks_run, checks_failed;

static void check(const char *name, int ok)
{
    checks_run++;
    if (!ok)
        checks_failed++;
    out("selftest: ");
    out(name);
    out(ok ? " ok" : " FAIL");
    nl();
}

/* Runs at boot and on demand. The exit marker is only for the on demand run so
 * that a boot does not look like a finished CI job. */
static void run_selftest(int with_exit_marker)
{
    char uid[33];
    uint32_t t0;
    uint32_t i;
    size_t j;
    int ram_ok = 1;

    checks_run = 0;
    checks_failed = 0;

    /* Reaching this line at all means the UART carried the response, so the
     * check is about saying so, not about proving it. */
    check("uart", 1);

    /* Bounded so a dead timer fails the check instead of hanging the board. */
    t0 = P->millis();
    for (i = 0; i < 20000000u && P->millis() - t0 < 3u; i++)
        ;
    check("timer", P->millis() - t0 >= 3u);

    P->uid(uid, sizeof uid);
    check("uid", uid[0] != 0 && !str_eq(uid, "000000000000000000000000"));

    for (j = 0; j < P->scratch_len; j++)
        P->scratch[j] = (uint8_t)(j * 7u + 1u);
    for (j = 0; j < P->scratch_len; j++)
        if (P->scratch[j] != (uint8_t)(j * 7u + 1u))
            ram_ok = 0;
    check("ram", ram_ok);

    /* Ports with no flash size register, such as the host port, report 0. */
    if (P->flash_kb)
        check("flash-size", 1);

    if (P->temp_mc) {
        int t = P->temp_mc();
        check("temp-sensor", t > -40000 && t < 125000);
    }

    out("selftest: ");
    out_u32((uint32_t)(checks_run - checks_failed));
    out(" passed, ");
    out_u32((uint32_t)checks_failed);
    out(" failed");
    nl();

    if (with_exit_marker) {
        /* srig run consumes this line and exits with N. */
        out("##srig-exit:");
        out_u32(checks_failed ? 1u : 0u);
        out("##");
        nl();
    }
}

/* ---- main loop ---- */

static void dispatch(char *line)
{
    char *arg = split(line);

    if (!*line)
        return;

    if (str_eq(line, "help"))
        cmd_help();
    else if (str_eq(line, "id"))
        cmd_id();
    else if (str_eq(line, "uptime")) {
        out("up=");
        out_u32(P->millis());
        out("ms");
        nl();
    } else if (str_eq(line, "echo")) {
        out(arg);
        nl();
    } else if (str_eq(line, "temp") && P->temp_mc) {
        out("temp=");
        out_milli(P->temp_mc());
        out("C");
        nl();
    } else if (str_eq(line, "bench"))
        cmd_bench();
    else if (str_eq(line, "selftest"))
        run_selftest(1);
    else if (str_eq(line, "reset")) {
        out("resetting");
        nl();
        P->reset();
    } else {
        out("err: unknown command '");
        out(line);
        out("', try help");
        nl();
    }
}

static void banner(void)
{
    char uid[33];

    nl();
    out("SiliconRig demo shell v" SHELL_VERSION " | board=");
    out(P->board);
    out(" | chip=");
    out(P->chip);
    nl();
    P->uid(uid, sizeof uid);
    out("uid=");
    out(uid);
    out(" flash=");
    out_u32(P->flash_kb);
    out("K ram=");
    out_u32(P->ram_kb);
    out("K");
    nl();
    run_selftest(0);
    out("Type 'help'. Heartbeat stops on first keypress.");
    nl();
}

static void heartbeat(void)
{
    out("[hb] ");
    out(P->board);
    out(" up=");
    out_u32(P->millis() / 1000u);
    out("s");
    if (P->temp_mc) {
        out(" t=");
        out_milli(P->temp_mc());
        out("C");
    }
    nl();
}

void shell_run(const shell_port *p)
{
    char line[LINE_MAX + 1];
    int len = 0;
    int prompted = 0;
    uint32_t last_input = 0;
    uint32_t hb_next;

    P = p;
    banner();
    hb_next = P->millis() + HEARTBEAT_MS;

    for (;;) {
        int c = P->get();

        if (c >= 0 && is_input_char(c)) {
            last_input = P->millis();
            if (!prompted) {
                prompted = 1; /* someone is there, stop announcing ourselves */
                out("> ");
            }
            if (c == '\r' || c == '\n') {
                nl();
                line[len] = 0;
                dispatch(line);
                len = 0;
                out("> ");
            } else if (c == 0x08 || c == 0x7f) {
                if (len) {
                    len--;
                    out("\b \b");
                }
            } else if (len < LINE_MAX) {
                line[len++] = (char)c;
                P->put((char)c);
            }
        }

        if (!prompted || P->millis() - last_input >= HEARTBEAT_RESUME_MS) {
            if ((int32_t)(P->millis() - hb_next) >= 0) {
                heartbeat();
                hb_next = P->millis() + HEARTBEAT_MS;
            }
        } else {
            hb_next = P->millis() + HEARTBEAT_MS;
        }
    }
}
