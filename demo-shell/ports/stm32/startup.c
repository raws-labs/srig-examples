/* Minimal Cortex-M startup shared by the STM32 ports.
 *
 * Only the 16 system vectors are populated: the shell drives every peripheral
 * by polling and takes just the SysTick interrupt. */

#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

int main(void);
void SysTick_Handler(void);

static void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata)
        *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss;)
        *dst++ = 0;

    main();
    for (;;)
        ;
}

static void Default_Handler(void)
{
    for (;;)
        ;
}

__attribute__((section(".isr_vector"), used)) void (*const vectors[16])(void) = {
    (void (*)(void)) & _estack,
    Reset_Handler,
    Default_Handler, /* NMI */
    Default_Handler, /* HardFault */
    Default_Handler, /* MemManage */
    Default_Handler, /* BusFault */
    Default_Handler, /* UsageFault */
    0, 0, 0, 0,
    Default_Handler, /* SVCall */
    Default_Handler, /* DebugMon */
    0,
    Default_Handler, /* PendSV */
    SysTick_Handler,
};
