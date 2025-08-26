/**
 * @file board.c
 * @brief Board-level startup helpers (banner, idle thread).
 * @version 1.0.2
 * @date 2025-08-26
 * author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 Translate comments to English.
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

/**
 * @brief Idle thread entry: performs background cleanup & optional power saving.
 */
static void idle_thread_entry(void)
{
    while (1)
    {
        /* Reclaim defunct threads. */
        s_cleanup_defunct_threads();

        /* Optionally insert low-power instruction (WFI). */
        /* __asm volatile ("wfi"); */
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
