/* Compile-only API/type probe. Never link or execute this translation unit. */
#include "start.h"
#include "start.h"

#if !defined(__ARM_ARCH_7M__) || !defined(__thumb__)
#error "The compile check must target Cortex-M3 Thumb code"
#endif

/* C99-compatible assertions, independent of host pointer and integer widths. */
#define CHECK_SIZE(type, bytes) \
    typedef char check_size_##type[sizeof(type) == (bytes) ? 1 : -1]
CHECK_SIZE(s_int8_t, 1);
CHECK_SIZE(s_uint8_t, 1);
CHECK_SIZE(s_int16_t, 2);
CHECK_SIZE(s_uint16_t, 2);
CHECK_SIZE(s_int32_t, 4);
CHECK_SIZE(s_uint32_t, 4);
CHECK_SIZE(s_int64_t, 8);
CHECK_SIZE(s_uint64_t, 8);
typedef char check_pointer_size[sizeof(void *) == 4 ? 1 : -1];
typedef char check_psp_offset[offsetof(s_thread, psp) == 0 ? 1 : -1];

static void entry(void) {}
static void timeout(void *parameter) { (void)parameter; }
#if START_USING_UDELAY
static void delay_hook(s_uint32_t us) { (void)us; }
#endif

void s_compile_api_check(void);
void s_compile_api_check(void)
{
    s_thread thread;
    s_timer timer;
    s_uint8_t stack[START_IDLE_STACK_SIZE];
    s_list list;
    s_uint32_t tick = s_tick_from_ms(1);
    s_status status;

    status = s_thread_init(&thread, entry, stack, sizeof(stack), 0, tick);
    status = s_timer_init(&timer, timeout, &thread, tick);
    s_list_init(&list);
    tick = s_irq_disable();
    s_irq_enable(tick);
    (void)__s_ffs(1);
    s_putc('a');
    s_printf("%s %d %c", "compile", 1, 'a');
    S_DEBUG_LOG(START_DEBUG_INFO, "%s", "compile");

#if START_USING_IPC
    status = s_ipc_suspend(&list, &thread, START_IPC_FLAG_FIFO);
    status = s_ipc_list_resume_all(&list);
#if START_USING_SEMAPHORE
    s_sem semaphore;
    status = s_sem_init(&semaphore, 1, START_IPC_FLAG_FIFO);
    status = s_sem_take(&semaphore, START_WAITING_NO);
    status = s_sem_release(&semaphore);
    status = s_sem_delete(&semaphore);
#endif
#if START_USING_MUTEX
    s_mutex mutex;
    status = s_mutex_init(&mutex, START_IPC_FLAG_PRIO);
    status = s_mutex_take(&mutex, START_WAITING_NO);
    status = s_mutex_release(&mutex);
    status = s_mutex_delete(&mutex);
#endif
#if START_USING_MESSAGEQUEUE
    s_msgqueue queue;
    s_uint32_t message = 0;
    s_uint8_t pool[START_MSGQ_POOL_SIZE(sizeof(message), 2)];
    status = s_msgqueue_init(&queue, pool, sizeof(message), sizeof(pool),
                             START_IPC_FLAG_FIFO);
    status = s_msgqueue_send(&queue, &message, sizeof(message));
    status = s_msgqueue_send_wait(&queue, &message, sizeof(message), START_WAITING_NO);
    status = s_msgqueue_urgent(&queue, &message, sizeof(message));
    status = s_msgqueue_recv(&queue, &message, sizeof(message), START_WAITING_NO);
    status = s_msgqueue_delete(&queue);
#endif
#endif
#if START_USING_UDELAY
    s_udelay_set_hook(delay_hook);
    s_udelay(1);
#endif
#if START_IDLE_HOOK_LIST_SIZE > 0
    status = s_idle_sethook(entry);
    status = s_idle_delhook(entry);
#endif
    (void)status;
}
