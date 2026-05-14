/**
 * @file board.c
 * @brief Board-level startup helpers (banner, idle thread).
 * @version 1.0.2
 * @date 2025-08-26
 * author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate comments to English.
 *     - 2026-05-13 1.0.3 StitchLilo626: Add idle thread hook list & SysTick us delay reference.
 */

#include "start.h"

#define START_BUILD_DATE __DATE__ " " __TIME__
#define START_COPYRIGHT  "Copyright (c) 2025 StaRT Project"

/**
 * @brief Weak banner printer (can be overridden by user).
 */
__weak void s_start_banner(void)
{
    s_printf("\r\n");
    s_printf("=================================================\r\n");
    s_printf("  StaRT RTOS - Lightweight Real-Time Operating System\r\n");
    s_printf("  Version    : %s\r\n", START_VERSION);
#if START_DEBUG
    s_printf("  Build Date : %s\r\n", START_BUILD_DATE);
#endif
    s_printf("  %s\r\n", START_COPYRIGHT);
    s_printf("=================================================\r\n");
    s_printf("\r\n");
}

/* Idle thread objects (statically allocated) */
static s_thread    idle_thread;
static s_uint8_t   idle_stack[START_IDLE_STACK_SIZE];
static volatile int idle_counter = 0;

#if START_IDLE_HOOK_LIST_SIZE > 0
/** Array of pointers to user-defined idle hook functions. */
static void (*idle_hook_list[START_IDLE_HOOK_LIST_SIZE])(void);

/**
 * @brief Register a hook function to be called in the idle thread.
 * @param hook Pointer to the hook function.
 * @return S_OK if registered successfully, S_ERR if the hook list is full.
 */
s_status s_idle_sethook(void (*hook)(void))
{
    s_uint32_t i;
    register s_uint32_t level = s_irq_disable();

    for (i = 0; i < START_IDLE_HOOK_LIST_SIZE; i++)
    {
        if (idle_hook_list[i] == NULL)
        {
            idle_hook_list[i] = hook;
            s_irq_enable(level);
            return S_OK;
        }
    }

    s_irq_enable(level);
    return S_ERR;
}

/**
 * @brief Remove a previously registered idle hook function.
 * @param hook Pointer to the hook function.
 * @return S_OK if removed successfully, S_ERR if not found.
 */
s_status s_idle_delhook(void (*hook)(void))
{
    s_uint32_t i;
    register s_uint32_t level = s_irq_disable();

    for (i = 0; i < START_IDLE_HOOK_LIST_SIZE; i++)
    {
        if (idle_hook_list[i] == hook)
        {
            idle_hook_list[i] = NULL;
            s_irq_enable(level);
            return S_OK;
        }
    }

    s_irq_enable(level);
    return S_ERR;
}
#endif /* START_IDLE_HOOK_LIST_SIZE > 0 */

/**
 * @brief Idle thread entry: performs background cleanup & optional power saving.
 */
static void idle_thread_entry(void)
{
    while (1)
    {
        /* Reclaim defunct threads. */
        s_cleanup_defunct_threads();

#if START_IDLE_HOOK_LIST_SIZE > 0
        s_uint32_t i;
        /* Execute all registered idle hooks */
        for (i = 0; i < START_IDLE_HOOK_LIST_SIZE; i++)
        {
            if (idle_hook_list[i] != NULL)
            {
                idle_hook_list[i]();
            }
        }
#else
        /* Optionally insert low-power instruction if no hooks are used. */
        /* __asm volatile ("wfi"); */
#endif
    }
}

/**
 * @brief Initialize and start idle thread (lowest priority).
 */
s_status s_idle_thread_init(void)
{
    s_status ret = s_thread_init(&idle_thread,
                                 idle_thread_entry,
                                 idle_stack,
                                 sizeof(idle_stack),
                                 START_THREAD_PRIORITY_MAX - 1,
                                 5);
    if (ret != S_OK)
        return ret;
    return s_thread_startup(&idle_thread);
}

/**
 * @brief Initialize kernel core subsystems.
 */
s_status s_start_init(void)
{
    s_sched_init();
    s_timer_list_init();
    s_idle_thread_init();
    s_start_banner();
    return S_OK;
}
