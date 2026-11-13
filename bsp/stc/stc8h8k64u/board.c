#include "board.h"
#include "start.h"

__sfr __at (0x8e) AUXR;
__sfr __at (0xa2) P_SW1;
__sfr __at (0xb1) P3M1;
__sfr __at (0xb2) P3M0;
__sfr __at (0xb7) IPH;
__sfr __at (0xd6) T2H;
__sfr __at (0xd7) T2L;
__sfr __at (0xe3) DPS;

#define TICK_COUNTS (START_STC8H_CLOCK_HZ / (12UL * START_TICK))
#define TICK_RELOAD (65536UL - TICK_COUNTS)
#define UART_BAUD 115200UL
#define UART_COUNTS ((START_STC8H_CLOCK_HZ + 2UL * UART_BAUD) / (4UL * UART_BAUD))
#define UART_RELOAD (65536UL - UART_COUNTS)

#if TICK_COUNTS < 1 || TICK_COUNTS > 65536 || UART_COUNTS < 1 || UART_COUNTS > 65536
#error "STC8H clock is outside the timer range"
#endif

/* SDCC calls this before initializing XDATA. */
unsigned char _sdcc_external_startup(void)
{
    AUXR &= ~0x02;
    DPS = 0;
    return 0;
}

void stc8h_board_init(void)
{
    IE = 0;
    TR0 = 0;
    TR1 = 0;
    TF0 = 0;
    TF1 = 0;
    IPH &= ~0x0a;
    PT0 = 0;
    PT1 = 1;

    /* Timer 1 mode 0 is 16-bit auto-reload on STC8H, with a /12 clock. */
    TMOD = 0;
    AUXR &= ~0x5e;
    TL1 = (unsigned char)TICK_RELOAD;
    TH1 = (unsigned char)(TICK_RELOAD >> 8);

    /* UART1 TX on P3.1, timer 2 as the baud-rate generator in 1T mode. */
    P_SW1 &= ~0xc0;
    P3M1 &= ~0x02;
    P3M0 |= 0x02;
    SCON = 0x40;
    T2L = (unsigned char)UART_RELOAD;
    T2H = (unsigned char)(UART_RELOAD >> 8);
    AUXR |= 0x15;
    TI = 1;
}

void stc8h_tick_start(void)
{
    ET0 = 1;
    ET1 = 1;
    TR1 = 1;
}

void stc8h_tick_isr(void) __interrupt(3)
{
    s_tick_increase();
}

void s_putc(char c)
{
    for (;;)
    {
        while (!TI) { }
        s_uint32_t level = s_irq_disable();
        if (TI)
        {
            TI = 0;
            SBUF = c;
            s_irq_enable(level);
            return;
        }
        s_irq_enable(level);
    }
}
