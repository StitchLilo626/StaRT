/* Core-only runtime regression test; no STM32 HAL or peripheral emulation. */
#include "start.h"

#define CHECK(condition) do { if (!(condition)) finish(1); } while (0)
#define ICSR (*(volatile s_uint32_t *)0xe000ed04)

static s_uint8_t stack_a[512] __attribute__((aligned(8)));
static s_uint8_t stack_b[512] __attribute__((aligned(8)));
static s_uint8_t stack_c[512] __attribute__((aligned(8)));
static void *sp_a, *sp_b, *sp_c;
s_uint32_t s_prev_thread_sp_p, s_next_thread_sp_p, s_interrupt_flag;
static volatile s_uint32_t initialized = 0x12345678;
static volatile s_uint32_t zeroed, system_initialized, constructed, phase;

static void finish(s_uint32_t status)
{
    static const char success[] = "PORT_SMOKE_PASS\n";
    static const char failure[] = "PORT_SMOKE_FAIL\n";
    s_uint32_t result[2] = {0x20026, status};
    register s_uint32_t operation __asm("r0") = 4; /* SYS_WRITE0 */
    register const void *argument __asm("r1") = status ? failure : success;
    __asm volatile("bkpt 0xab" : "+r"(operation) : "r"(argument) : "memory");
    operation = 0x20; /* SYS_EXIT_EXTENDED */
    argument = result;
    __asm volatile("bkpt 0xab" : "+r"(operation) : "r"(argument) : "memory");
    for (;;) {}
}

void SystemInit(void) { system_initialized = 1; }
static void __attribute__((constructor)) constructor(void)
{
    constructed = system_initialized && initialized == 0x12345678 && zeroed == 0;
}

static s_uint32_t primask(void)
{
    s_uint32_t value;
    __asm volatile("mrs %0, primask" : "=r"(value));
    return value;
}

static void check_thread_stack(s_uint8_t *stack)
{
    s_uint32_t psp, control;
    __asm volatile("mrs %0, psp" : "=r"(psp));
    __asm volatile("mrs %0, control" : "=r"(control));
    CHECK((control & 2) && psp % 8 == 0);
    CHECK(psp >= (s_uint32_t)stack && psp <= (s_uint32_t)(stack + 512));
    CHECK(primask() == 0);
}

void smoke_request_switch(void)
{
    s_uint32_t level = s_irq_disable();
    s_normal_switch_task((s_uint32_t)&sp_a, (s_uint32_t)&sp_b);
    CHECK(primask() == 1);
    s_normal_switch_task((s_uint32_t)&sp_b, (s_uint32_t)&sp_c);
    CHECK(s_prev_thread_sp_p == (s_uint32_t)&sp_a);
    CHECK(s_next_thread_sp_p == (s_uint32_t)&sp_c && primask() == 1);
    s_irq_enable(level);
    __asm volatile("dsb\nisb" ::: "memory");
    CHECK(phase == 2);
}

int smoke_context_probe(void);
static void thread_a(void)
{
    check_thread_stack(stack_a);
    CHECK(s_interrupt_flag == 0 && s_prev_thread_sp_p == 0);
    CHECK(constructed == 1);
    phase = 1;
    CHECK(smoke_context_probe() == 1);
    check_thread_stack(stack_a);
    CHECK(phase == 2 && s_interrupt_flag == 0);
    /* An unrelated/spurious PendSV request must leave EXC_RETURN intact. */
    ICSR = 1u << 28;
    __asm volatile("dsb\nisb" ::: "memory");
    check_thread_stack(stack_a);
    finish(0);
}

static void thread_b(void) { finish(2); } /* Replaced pending target must not run. */
static void thread_c(void)
{
    check_thread_stack(stack_c);
    CHECK(phase == 1);
    CHECK((s_uint32_t)sp_a >= (s_uint32_t)stack_a);
    CHECK((s_uint32_t)sp_a < (s_uint32_t)(stack_a + sizeof(stack_a)));
    phase = 2;
    s_normal_switch_task((s_uint32_t)&sp_c, (s_uint32_t)&sp_a);
    for (;;) {}
}

void SysTick_Handler(void) { finish(3); }
void s_thread_exit(void) { finish(4); }
int main(void)
{
    CHECK(initialized == 0x12345678 && zeroed == 0 && constructed == 1);
    CHECK(primask() == 0);
    s_uint32_t outer = s_irq_disable();
    s_uint32_t inner = s_irq_disable();
    CHECK(outer == 0 && inner == 1);
    s_irq_enable(inner);
    CHECK(primask() == 1);
    s_irq_enable(outer);
    CHECK(primask() == 0);
    sp_a = s_stack_init(thread_a, stack_a + sizeof(stack_a));
    sp_b = s_stack_init(thread_b, stack_b + sizeof(stack_b));
    sp_c = s_stack_init(thread_c, stack_c + sizeof(stack_c));
    s_first_switch_task((s_uint32_t)&sp_a);
    finish(5);
}
