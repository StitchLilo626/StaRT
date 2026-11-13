#ifndef START_NUCLEO_F439ZI_BOARD_H
#define START_NUCLEO_F439ZI_BOARD_H

#include <stdint.h>

#define BOARD_LD1 (1UL << 0)
#define BOARD_LD2 (1UL << 7)

extern uint32_t SystemCoreClock;

void board_init(void);
void board_tick_start(void);
void board_led_write(uint32_t pin, int on);

#endif
