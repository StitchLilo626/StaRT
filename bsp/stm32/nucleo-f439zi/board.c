#include "board.h"
#include "cpuport.h"
#include "start.h"

/* STM32F439 peripheral registers and Cortex-M system registers. */
#define REG32(addr) (*(volatile uint32_t *)(addr))
#define FLASH_ACR   REG32(0x40023c00UL)
#define RCC_CR      REG32(0x40023800UL)
#define RCC_PLLCFGR REG32(0x40023804UL)
#define RCC_CFGR    REG32(0x40023808UL)
#define RCC_AHB1ENR REG32(0x40023830UL)
#define RCC_APB1ENR REG32(0x40023840UL)
#define PWR_CR      REG32(0x40007000UL)
#define PWR_CSR     REG32(0x40007004UL)
#define GPIOB_MODER REG32(0x40020400UL)
#define GPIOB_BSRR  REG32(0x40020418UL)
#define GPIOD_MODER REG32(0x40020c00UL)
#define GPIOD_AFRH  REG32(0x40020c24UL)
#define USART3_SR   REG32(0x40004800UL)
#define USART3_DR   REG32(0x40004804UL)
#define USART3_BRR  REG32(0x40004808UL)
#define USART3_CR1  REG32(0x4000480cUL)
#define SYST_CSR    REG32(0xe000e010UL)
#define SYST_RVR    REG32(0xe000e014UL)
#define SYST_CVR    REG32(0xe000e018UL)
#define SCB_SHPR3   REG32(0xe000ed20UL)

#define HSI_HZ        16000000UL
#define PLL_HZ        168000000UL
#define UART_BAUD     115200UL
#define PLL_READY     (1UL << 25)
#define VOS_READY     (1UL << 14)
#define APB_DIVIDERS  ((5UL << 10) | (4UL << 13)) /* APB1 /4, APB2 /2 */

uint32_t SystemCoreClock = HSI_HZ;

void SystemInit(void)
{
    s_cm4f_fpu_init();
    /* Reset starts on HSI at 16 MHz. Configure the PLL without using HSE. */
    RCC_APB1ENR |= 1UL << 28; /* PWR */
    (void)RCC_APB1ENR;
    PWR_CR |= 3UL << 14; /* Voltage scale 1 */
    FLASH_ACR = (1UL << 10) | (1UL << 9) | 5UL; /* I/D cache, 5 wait states */
    while ((FLASH_ACR & 7UL) != 5UL) {}

    /* PLL input 1 MHz, VCO 336 MHz, PLLP /2 and PLLQ /7. */
    RCC_PLLCFGR = 16UL | (336UL << 6) | (7UL << 24);
    RCC_CR |= 1UL << 24;
    for (uint32_t wait = 0; wait < HSI_HZ; ++wait)
    {
        if ((RCC_CR & PLL_READY) && (PWR_CSR & VOS_READY))
        {
            RCC_CFGR = APB_DIVIDERS | 2UL;
            while ((RCC_CFGR & (3UL << 2)) != (2UL << 2)) {}
            SystemCoreClock = PLL_HZ;
            return;
        }
    }
    /* Keep the reset clock and prescalers if the PLL does not lock. */
    RCC_CR &= ~(1UL << 24);
}

void board_init(void)
{
    uint32_t apb1_hz = SystemCoreClock == PLL_HZ ? PLL_HZ / 4 : HSI_HZ;

    RCC_AHB1ENR |= (1UL << 1) | (1UL << 3); /* GPIOB, GPIOD */
    RCC_APB1ENR |= 1UL << 18; /* USART3 */
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;

    /* LD1 / LD2 are active high, on PB0 / PB7. Set output level first. */
    GPIOB_BSRR = (BOARD_LD1 | BOARD_LD2) << 16;
    GPIOB_MODER = (GPIOB_MODER & ~((3UL << 0) | (3UL << 14))) |
                  (1UL << 0) | (1UL << 14);

    /* ST-LINK VCP on PD8 (TX) and PD9 (RX), AF7. */
    GPIOD_AFRH = (GPIOD_AFRH & ~0xffUL) | 0x77UL;
    GPIOD_MODER = (GPIOD_MODER & ~(0xfUL << 16)) | (0xaUL << 16);
    USART3_BRR = (apb1_hz + UART_BAUD / 2) / UART_BAUD;
    USART3_CR1 = (1UL << 13) | (1UL << 3) | (1UL << 2); /* UE, TE, RE */
}

void board_tick_start(void)
{
    /* Keep SysTick above the lowest-priority PendSV set by the CPU port. */
    SCB_SHPR3 = (SCB_SHPR3 & 0x00ffffffUL) | 0x80000000UL;
    SYST_RVR = SystemCoreClock / START_TICK - 1;
    SYST_CVR = 0;
    SYST_CSR = 7; /* Core clock, interrupt and counter enable */
}

void SysTick_Handler(void)
{
    s_tick_increase();
}

void board_led_write(uint32_t pin, int on)
{
    GPIOB_BSRR = on ? pin : pin << 16;
}

void s_putc(char c)
{
    while (!(USART3_SR & (1UL << 7))) {}
    USART3_DR = (unsigned char)c;
}
