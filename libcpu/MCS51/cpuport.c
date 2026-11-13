#include "start.h"
#include <8052.h>

#if !defined(__SDCC_mcs51) || !defined(__SDCC_STACK_AUTO)
#error "The MCS-51 port requires SDCC with --stack-auto"
#endif

/* Shared with context.asm. Interrupts are disabled when writing these slots. */
volatile __data s_uint16_t s_mcs51_outgoing;
volatile __data s_uint16_t s_mcs51_incoming;
volatile __data s_uint8_t s_mcs51_pending;

int __s_ffs(s_uint32_t value)
{
    int bit_index = 1;
    if (!value)
        return 0;
    while (!(value & 1UL))
    {
        value >>= 1;
        ++bit_index;
    }
    return bit_index;
}

s_uint8_t *s_stack_init(s_thread_entry_t entry, s_uint8_t *stackaddr, s_uint32_t size)
{
    enum { HEADER_SIZE = 2, INITIAL_FRAME_SIZE = 19 };
    __xdata s_uint8_t *image = (__xdata s_uint8_t *)stackaddr;
    s_uint8_t *generic_image = image;
    s_uint16_t entry_address = (s_uint16_t)entry;
    s_uint16_t exit_address = (s_uint16_t)s_thread_exit;

    if (size < HEADER_SIZE + INITIAL_FRAME_SIZE || generic_image != stackaddr ||
        (s_uint16_t)image == 0)
        return NULL;

    image[0] = size > 255UL + HEADER_SIZE ? 255 : (s_uint8_t)(size - HEADER_SIZE);
    image[1] = INITIAL_FRAME_SIZE;
    /* RETI consumes the entry PC. A thread returning from entry calls s_thread_exit. */
    image[2] = (s_uint8_t)exit_address;
    image[3] = (s_uint8_t)(exit_address >> 8);
    image[4] = (s_uint8_t)entry_address;
    image[5] = (s_uint8_t)(entry_address >> 8);
    for (s_uint8_t i = 6; i < HEADER_SIZE + INITIAL_FRAME_SIZE; ++i)
        image[i] = 0;

    return image;
}

void s_first_switch_task(s_switch_arg_t next)
{
    EA = 0;
    s_mcs51_outgoing = 0;
    s_mcs51_incoming = (s_uint16_t)(__xdata s_uint8_t *)*next;
    s_mcs51_pending = 1;
    TF0 = 1;
    EA = 1;
    for (;;) { }
}

void s_normal_switch_task(s_switch_arg_t prev, s_switch_arg_t next)
{
    s_uint32_t level = s_irq_disable();
    if (!s_mcs51_pending)
        s_mcs51_outgoing = prev ? (s_uint16_t)(__xdata s_uint8_t *)*prev : 0;
    s_mcs51_incoming = (s_uint16_t)(__xdata s_uint8_t *)*next;
    s_mcs51_pending = 1;
    TF0 = 1;
    s_irq_enable(level);
}
