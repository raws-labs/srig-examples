/* Portable interactive demo shell for SiliconRig boards.
 *
 * A port supplies the functions below and calls shell_run(); everything a user
 * sees is in shell.c so all board types behave identically. */
#ifndef SRIG_SHELL_H
#define SRIG_SHELL_H

#include <stddef.h>
#include <stdint.h>

#define SHELL_VERSION "1"

typedef struct {
    const char *board; /* SiliconRig board type, e.g. "stm32-h753" */
    const char *chip;  /* marketing chip name, e.g. "STM32H753ZI" */

    uint32_t flash_kb;
    uint32_t ram_kb;

    void (*put)(char c);
    int (*get)(void);          /* next input byte, or -1 if none pending */
    uint32_t (*millis)(void);  /* monotonic, wraps after 49 days */
    int (*temp_mc)(void); /* on-die sensor, milli degrees C; NULL if none */
    void (*uid)(char *out, size_t n); /* hex chip id, NUL terminated */
    void (*reset)(void);

    /* Scratch buffer the RAM selftest writes over. */
    uint8_t *scratch;
    size_t scratch_len;
} shell_port;

/* Runs forever. */
void shell_run(const shell_port *p);

#endif
