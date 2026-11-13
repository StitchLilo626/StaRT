#ifndef START_STC8H_BOARD_H
#define START_STC8H_BOARD_H

#include <8052.h>

/* SDCC emits the interrupt vectors in the translation unit defining main. */
void s_mcs51_switch_isr(void) __interrupt(1) __naked;
void stc8h_tick_isr(void) __interrupt(3);

void stc8h_board_init(void);
void stc8h_tick_start(void);

#endif
