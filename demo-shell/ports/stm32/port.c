/* STM32 port of the demo shell. One file, two targets, selected with
 * -DTARGET_H753 or -DTARGET_F446.
 *
 * Everything runs off the reset default HSI clock with no PLL, so there is no
 * clock tree to get wrong and the UART divisor is a compile time constant.
 * Both boards are Nucleos: the UART below is the one wired to the onboard
 * ST-LINK virtual COM port, which is what the pod reads. */

#include "../../src/shell.h"

#define REG(a) (*(volatile uint32_t *)(a))
#define REG16(a) (*(volatile uint16_t *)(a))

#if defined(TARGET_H753)

#define BOARD "stm32-h753"
#define CHIP "STM32H753ZI"
#define CPU_HZ 64000000u /* HSI, HSIDIV /1 */
#define RAM_KB 1024u

#define RCC 0x58024400u
#define RCC_CR (RCC + 0x00)
#define RCC_CFGR (RCC + 0x10)
#define RCC_D1CFGR (RCC + 0x18)
#define RCC_D2CFGR (RCC + 0x1C)
#define RCC_D3CFGR (RCC + 0x20)
#define RCC_AHB4ENR (RCC + 0xE0)
#define RCC_APB1LENR (RCC + 0xE8)

#define GPIO_UART 0x58020C00u /* GPIOD, USART3 on PD8 and PD9 */
#define UART_TX_PIN 8u
#define UART_RX_PIN 9u

#define UART 0x40004800u /* USART3 */
#define UART_CR1 (UART + 0x00)
#define UART_BRR (UART + 0x0C)
#define UART_ISR (UART + 0x1C)
#define UART_RDR (UART + 0x24)
#define UART_TDR (UART + 0x28)
#define UART_TXE (1u << 7)
#define UART_RXNE (1u << 5)
#define UART_CR1_EN ((1u << 0) | (1u << 2) | (1u << 3)) /* UE, RE, TE */

#define UID_BASE 0x1FF1E800u
#define FLASH_SIZE_REG 0x1FF1E880u

/* ADC3 owns the temperature sensor on this part, and its kernel clock has to
 * come from per_ck because no PLL is running. */
#define RCC_D3CCIPR (RCC + 0x058)
#define ADC 0x58026000u
#define ADC_CCR 0x58026308u /* common block is CSR, reserved, then CCR */
#define TS_CAL1 0x1FF1E820u /* raw at 30 C, 3.3 V, 16 bit */
#define TS_CAL2 0x1FF1E840u /* raw at 110 C */

#elif defined(TARGET_F446)

#define BOARD "stm32-f446"
#define CHIP "STM32F446RE"
#define CPU_HZ 16000000u /* HSI */
#define RAM_KB 128u

#define RCC 0x40023800u
#define RCC_CR (RCC + 0x00)
#define RCC_CFGR (RCC + 0x08)
#define RCC_AHB1ENR (RCC + 0x30)
#define RCC_APB1ENR (RCC + 0x40)

#define GPIO_UART 0x40020000u /* GPIOA, USART2 on PA2 and PA3 */
#define UART_TX_PIN 2u
#define UART_RX_PIN 3u

#define UART 0x40004400u /* USART2 */
#define UART_SR (UART + 0x00)
#define UART_DR (UART + 0x04)
#define UART_BRR (UART + 0x08)
#define UART_CR1 (UART + 0x0C)
#define UART_TXE (1u << 7)
#define UART_RXNE (1u << 5)
#define UART_CR1_EN ((1u << 13) | (1u << 2) | (1u << 3)) /* UE, RE, TE */

#define UID_BASE 0x1FFF7A10u
#define FLASH_SIZE_REG 0x1FFF7A22u

#define RCC_APB2ENR (RCC + 0x44)
#define ADC 0x40012000u
#define ADC_CCR 0x40012304u /* common block is CSR, then CCR */
#define TS_CAL1 0x1FFF7A2Cu /* raw at 30 C, 3.3 V, 12 bit */
#define TS_CAL2 0x1FFF7A2Eu /* raw at 110 C */

#else
#error "define TARGET_H753 or TARGET_F446"
#endif

#define GPIO_MODER 0x00u
#define GPIO_AFRL 0x20u
#define GPIO_AFRH 0x24u
#define UART_AF 7u

#define SYST_CSR 0xE000E010u
#define SYST_RVR 0xE000E014u
#define SYST_CVR 0xE000E018u
#define SCB_AIRCR 0xE000ED0Cu

#define BAUD 115200u

/* Both parts put the sensor on channel 18 and both store factory calibration
 * points in system memory, so only the ADC bring up differs. */
#define TEMP_CHANNEL 18u

static volatile uint32_t tick_ms;
static uint8_t scratch[1024];

void SysTick_Handler(void)
{
    tick_ms++;
}

static void gpio_mode(uint32_t port, uint32_t pin, uint32_t mode)
{
    REG(port + GPIO_MODER) =
        (REG(port + GPIO_MODER) & ~(3u << (pin * 2))) | (mode << (pin * 2));
}

static void gpio_af(uint32_t port, uint32_t pin, uint32_t af)
{
    uint32_t reg = port + (pin < 8 ? GPIO_AFRL : GPIO_AFRH);
    uint32_t shift = (pin & 7u) * 4u;

    REG(reg) = (REG(reg) & ~(0xFu << shift)) | (af << shift);
    gpio_mode(port, pin, 2u); /* alternate function */
}

static void clock_init(void)
{
#if defined(TARGET_H753)
    REG(RCC_CR) |= 1u;                /* HSION */
    while (!(REG(RCC_CR) & (1u << 2))) /* HSIRDY */
        ;
    REG(RCC_CR) &= ~(3u << 3); /* HSIDIV /1 -> 64 MHz */
    REG(RCC_CFGR) &= ~7u;      /* sys_ck = HSI */
    while (REG(RCC_CFGR) & (7u << 3))
        ;
    REG(RCC_D1CFGR) = 0; /* no D1, D2, D3 prescaling */
    REG(RCC_D2CFGR) = 0;
    REG(RCC_D3CFGR) = 0;
    REG(RCC_AHB4ENR) |= (1u << 3); /* GPIOD */
    REG(RCC_APB1LENR) |= (1u << 18);           /* USART3 */
#else
    REG(RCC_CR) |= 1u;                 /* HSION */
    while (!(REG(RCC_CR) & (1u << 1))) /* HSIRDY */
        ;
    REG(RCC_CFGR) = 0;              /* sys_ck = HSI, all prescalers /1 */
    REG(RCC_AHB1ENR) |= (1u << 0);  /* GPIOA */
    REG(RCC_APB1ENR) |= (1u << 17); /* USART2 */
#endif
}

static void uart_init(void)
{
    gpio_af(GPIO_UART, UART_TX_PIN, UART_AF);
    gpio_af(GPIO_UART, UART_RX_PIN, UART_AF);
    REG(UART_CR1) = 0;
    REG(UART_BRR) = (CPU_HZ + BAUD / 2u) / BAUD;
    REG(UART_CR1) = UART_CR1_EN;
}

static void port_put(char c)
{
#if defined(TARGET_H753)
    while (!(REG(UART_ISR) & UART_TXE))
        ;
    REG(UART_TDR) = (uint32_t)(uint8_t)c;
#else
    while (!(REG(UART_SR) & UART_TXE))
        ;
    REG(UART_DR) = (uint32_t)(uint8_t)c;
#endif
}

static int port_get(void)
{
#if defined(TARGET_H753)
    if (!(REG(UART_ISR) & UART_RXNE))
        return -1;
    return (int)(REG(UART_RDR) & 0xFFu);
#else
    if (!(REG(UART_SR) & UART_RXNE))
        return -1;
    return (int)(REG(UART_DR) & 0xFFu);
#endif
}


/* Nothing in ADC bring up may hang: a silent board is the failure this whole
 * image exists to prevent, so every wait is bounded and a timeout just means
 * the shell runs without the temp command. */
#define SPIN_LIMIT 2000000u

static int wait_bit(uint32_t reg, uint32_t mask, int want_set)
{
    uint32_t n;
    for (n = 0; n < SPIN_LIMIT; n++)
        if (!!(REG(reg) & mask) == !!want_set)
            return 1;
    return 0;
}

static void delay_us(uint32_t us)
{
    uint32_t n = us * (CPU_HZ / 4000000u);
    while (n--)
        __asm__ volatile("nop");
}

#if defined(TARGET_H753)

#define ADC_ISR (ADC + 0x00)
#define ADC_CR (ADC + 0x08)
#define ADC_CFGR (ADC + 0x0C)
#define ADC_SMPR2 (ADC + 0x18)
#define ADC_PCSEL (ADC + 0x1C)
#define ADC_SQR1 (ADC + 0x30)
#define ADC_DR (ADC + 0x40)

static int adc_init(void)
{
    REG(RCC_D3CCIPR) = (REG(RCC_D3CCIPR) & ~(3u << 16)) | (2u << 16); /* per_ck */
    REG(RCC_AHB4ENR) |= (1u << 24);                                   /* ADC3 */

    /* per_ck is 64 MHz, so divide by 16: at 4 MHz the ADC needs no boost and
     * BOOST stays at its reset value on both silicon revisions. */
    REG(ADC_CCR) = (REG(ADC_CCR) & ~(0xFu << 18)) | (7u << 18);
    REG(ADC_CCR) |= (1u << 23); /* VSENSEEN */

    REG(ADC_CR) &= ~(1u << 29); /* leave deep power down */
    REG(ADC_CR) |= (1u << 28);  /* voltage regulator */
    delay_us(20);

    REG(ADC_CR) |= (1u << 31); /* calibrate */
    if (!wait_bit(ADC_CR, 1u << 31, 0))
        return 0;

    REG(ADC_ISR) = 1u; /* clear ADRDY */
    REG(ADC_CR) |= 1u; /* enable */
    if (!wait_bit(ADC_ISR, 1u, 1))
        return 0;

    REG(ADC_PCSEL) |= (1u << TEMP_CHANNEL);
    REG(ADC_SMPR2) |= (7u << ((TEMP_CHANNEL - 10u) * 3u)); /* longest sample */
    REG(ADC_SQR1) = TEMP_CHANNEL << 6;                     /* one conversion */
    delay_us(200);                                         /* sensor startup */
    return 1;
}

static int adc_read(uint32_t *out)
{
    REG(ADC_CR) |= (1u << 2); /* ADSTART */
    if (!wait_bit(ADC_ISR, 1u << 2, 1))
        return 0;
    *out = REG(ADC_DR);
    return 1;
}

#else

#define ADC_SR (ADC + 0x00)
#define ADC_CR2 (ADC + 0x08)
#define ADC_SMPR1 (ADC + 0x0C)
#define ADC_SQR1 (ADC + 0x2C)
#define ADC_SQR3 (ADC + 0x34)
#define ADC_DR (ADC + 0x4C)

static int adc_init(void)
{
    REG(RCC_APB2ENR) |= (1u << 8); /* ADC1 */
    REG(ADC_CCR) |= (1u << 23);    /* TSVREFE */

    REG(ADC_SMPR1) |= (7u << ((TEMP_CHANNEL - 10u) * 3u)); /* 480 cycles */
    REG(ADC_SQR1) = 0;                                     /* one conversion */
    REG(ADC_SQR3) = TEMP_CHANNEL;
    REG(ADC_CR2) |= 1u; /* ADON */
    delay_us(200);      /* sensor startup */
    return 1;
}

static int adc_read(uint32_t *out)
{
    REG(ADC_CR2) |= (1u << 30); /* SWSTART */
    if (!wait_bit(ADC_SR, 1u << 1, 1))
        return 0;
    *out = REG(ADC_DR) & 0xFFFFu;
    return 1;
}

#endif

/* Two point factory calibration: 30 C and 110 C. */
static int port_temp_mc(void)
{
    uint32_t raw;
    int32_t c1 = (int32_t)REG16(TS_CAL1);
    int32_t c2 = (int32_t)REG16(TS_CAL2);

    if (!adc_read(&raw) || c2 <= c1)
        return -273000; /* conversion failed or no usable calibration data */
    return (int)(((int32_t)raw - c1) * 80000 / (c2 - c1) + 30000);
}

static uint32_t port_millis(void)
{
    return tick_ms;
}

/* 96 bit device id, most significant word first. */
static void port_uid(char *out, size_t n)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;
    int w;

    for (w = 2; w >= 0; w--) {
        uint32_t v = REG(UID_BASE + (uint32_t)w * 4u);
        int nib;
        for (nib = 7; nib >= 0; nib--) {
            if (pos + 1 >= n)
                break;
            out[pos++] = hex[(v >> (nib * 4)) & 0xFu];
        }
    }
    out[pos] = 0;
}

static void port_reset(void)
{
    REG(SCB_AIRCR) = 0x05FA0004u;
    for (;;)
        ;
}

int main(void)
{
    static shell_port port;

    clock_init();
    uart_init();

    REG(SYST_RVR) = CPU_HZ / 1000u - 1u;
    REG(SYST_CVR) = 0;
    REG(SYST_CSR) = 7u; /* core clock, interrupt, enable */

    port.board = BOARD;
    port.chip = CHIP;
    port.flash_kb = REG16(FLASH_SIZE_REG);
    port.ram_kb = RAM_KB;
    port.put = port_put;
    port.get = port_get;
    port.millis = port_millis;
    port.temp_mc = adc_init() ? port_temp_mc : 0;
    port.uid = port_uid;
    port.reset = port_reset;
    port.scratch = scratch;
    port.scratch_len = sizeof scratch;

    shell_run(&port);
    return 0;
}
