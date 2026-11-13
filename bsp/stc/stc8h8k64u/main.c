#include "board.h"
#include "start.h"

static s_thread fast_thread;
static s_thread slow_thread;
static s_sem sample_ready;
static __xdata s_uint8_t fast_stack[START_STC8H_STACK_IMAGE_SIZE];
static __xdata s_uint8_t slow_stack[START_STC8H_STACK_IMAGE_SIZE];

static void fast_entry(void)
{
    for (;;)
    {
        if (s_sem_take(&sample_ready, START_WAITING_FOREVER) == S_OK)
            s_putc('A');
    }
}

static void slow_entry(void)
{
    for (;;)
    {
        s_putc('B');
        s_sem_release(&sample_ready);
        s_mdelay(1000);
    }
}

void main(void)
{
    stc8h_board_init();
    if (s_start_init() == S_OK &&
        s_sem_init(&sample_ready, 0, START_IPC_FLAG_FIFO) == S_OK &&
        s_thread_init(&fast_thread, fast_entry, fast_stack, sizeof(fast_stack), 0, 2) == S_OK &&
        s_thread_init(&slow_thread, slow_entry, slow_stack, sizeof(slow_stack), 1, 2) == S_OK &&
        s_thread_startup(&fast_thread) == S_OK &&
        s_thread_startup(&slow_thread) == S_OK)
    {
        stc8h_tick_start();
        s_sched_start();
    }
    for (;;) { }
}
