#include "board.h"
#include "start.h"

static s_thread first;
static s_thread second;
static s_thread high;
static s_sem semaphore;
static __xdata s_uint8_t first_stack[START_STC8H_STACK_IMAGE_SIZE];
static __xdata s_uint8_t second_stack[START_STC8H_STACK_IMAGE_SIZE];
static __xdata s_uint8_t high_stack[START_STC8H_STACK_IMAGE_SIZE];
static __xdata s_uint8_t short_stack[20];
static __data s_uint8_t wrong_space[1];
static volatile __xdata __at (0x1fff) unsigned char simulator;
static volatile unsigned char phase;
static volatile unsigned char high_runs;
static volatile unsigned char idle_seen;
static s_thread short_thread;
static __xdata s_uint8_t short_context[23];
static const __code unsigned char startup_value = 0x5a;
static __xdata unsigned char initialized_value = 0xa5;

static void report(char result)
{
    simulator = 'p';
    simulator = result;
    simulator = 's';
    for (;;) { }
}

void s_putc(char c)
{
    (void)c;
}

/* uCsim models the classic 8052 timer, so explicitly reload its mode 1. */
static void reload_tick(void)
{
    TH1 = 0xdc;
    TL1 = 0;
}

void stc8h_tick_isr(void) __interrupt(3)
{
    TR1 = 0;
    reload_tick();
    TR1 = 1;
    if (s_thread_get() && s_thread_get()->current_priority == START_THREAD_PRIORITY_MAX - 1)
        idle_seen = 1;
    s_tick_increase();
}

static void high_entry(void)
{
    ++high_runs;
    s_thread_sleep(3);
    ++high_runs;
    /* Returning through the synthetic frame must run s_thread_exit(). */
}

static void short_entry(void)
{
    s_thread_sleep(10);
    report('X');
}

static void first_entry(void)
{
    volatile unsigned char local = 0x5b;
    s_uint32_t before;
    if (phase != 0 || high_runs != 1 || !EA)
        report('A');
    s_uint32_t level = s_irq_disable();
    s_normal_switch_task(&first.psp, &second.psp);
    s_normal_switch_task(&second.psp, &first.psp);
    if (EA)
        report('N');
    s_irq_enable(level);
    TF0 = 1; /* No pending request: the ISR must leave this context intact. */
    phase = 1;
    while (phase == 1) { } /* Only a timer interrupt can yield to second. */
    if (phase != 2 || local != 0x5b)
        report('B');
    before = s_tick_get();
    s_thread_sleep(4);
    if (s_tick_get() - before < 4 || high_runs != 2 || local != 0x5b)
        report('C');
    phase = 3;
    s_thread_yield();
    if (phase != 4 || local != 0x5b)
        report('D');
    phase = 5;
    s_thread_sleep(4);
    if (!idle_seen || high.status != START_THREAD_DELETED)
        report('E');
    if (s_sem_take(&semaphore, START_WAITING_NO) != S_ERR ||
        s_sem_take(&semaphore, 6) != S_OK ||
        !s_list_isempty(&first.timer.row[0]) ||
        s_sem_take(&semaphore, START_WAITING_NO) != S_ERR)
        report('O');
    before = s_tick_get();
    if (s_sem_take(&semaphore, 8) != S_ERR || s_tick_get() - before < 8 ||
        !s_list_isempty(&semaphore.parent.suspend_thread))
        report('Q');
    phase = 6;
    if (s_sem_take(&semaphore, START_WAITING_FOREVER) != S_OK || phase != 7)
        report('R');
    if (s_sem_take(&semaphore, 8) != S_ERR || phase != 8 ||
        !s_list_isempty(&first.timer.row[0]) ||
        s_sem_take(&semaphore, START_WAITING_NO) != S_DELETED)
        report('S');
    if (s_thread_init(&short_thread, short_entry, short_context,
                      sizeof(short_context), 0, 2) != S_OK ||
        s_thread_startup(&short_thread) != S_OK)
        report('F');
    simulator = 'p';
    simulator = 'P';
    s_thread_sleep(4);
    s_sched_switch();
    report('X');
}

static void second_entry(void)
{
    volatile unsigned char local = 0xac;
    if (phase != 1)
        report('G');
    phase = 2;
    while (phase == 2) { }
    if (phase != 3 || local != 0xac)
        report('H');
    phase = 4;
    while (phase == 4) { }
    if (phase != 5 || local != 0xac)
        report('I');
    s_thread_sleep(6);
    if (s_list_isempty(&semaphore.parent.suspend_thread) ||
        s_sem_release(&semaphore) != S_OK)
        report('T');
    while (phase != 6)
        s_thread_sleep(1);
    phase = 7;
    if (s_list_isempty(&semaphore.parent.suspend_thread) ||
        s_sem_release(&semaphore) != S_OK)
        report('U');
    s_thread_sleep(2);
    phase = 8;
    if (s_list_isempty(&semaphore.parent.suspend_thread) ||
        s_sem_delete(&semaphore) != S_OK)
        report('V');
    s_thread_sleep(100);
    report('X');
}

void main(void)
{
    if (initialized_value != (unsigned char)~startup_value || phase || high_runs)
        report('J');
    if (s_start_init() != S_OK ||
        s_sem_init(&semaphore, SEM_VALUE_MAX, START_IPC_FLAG_FIFO) != S_OK ||
        s_sem_release(&semaphore) != S_ERR ||
        s_sem_take(&semaphore, START_WAITING_NO) != S_OK ||
        semaphore.count != SEM_VALUE_MAX - 1 ||
        s_sem_init(&semaphore, 0, START_IPC_FLAG_FIFO) != S_OK ||
        s_thread_init(&first, first_entry, short_stack, sizeof(short_stack), 1, 2) != S_INVALID ||
        s_thread_init(&first, first_entry, &wrong_space, 256, 1, 2) != S_INVALID ||
        s_thread_init(&first, first_entry, first_stack, sizeof(first_stack), 1, 2) != S_OK ||
        s_thread_init(&second, second_entry, second_stack, sizeof(second_stack), 1, 2) != S_OK ||
        s_thread_init(&high, high_entry, high_stack, sizeof(high_stack), 0, 2) != S_OK ||
        s_thread_startup(&first) != S_OK ||
        s_thread_startup(&second) != S_OK ||
        s_thread_startup(&high) != S_OK)
        report('K');

    EA = 1;
    s_uint32_t outer = s_irq_disable();
    s_uint32_t inner = s_irq_disable();
    s_irq_enable(inner);
    if (outer != 1 || inner != 0 || EA)
        report('L');
    s_irq_enable(outer);
    if (!EA || __s_ffs(0x80000000UL) != 32 || __s_ffs(0) != 0)
        report('M');
    EA = 0;
    TMOD = 0x10;
    reload_tick();
    PT1 = 1;
    ET1 = 1;
    ET0 = 1;
    TR1 = 1;
    s_sched_start();
    report('X');
}
