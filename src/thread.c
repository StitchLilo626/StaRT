/**
 * @file thread.c
 * @brief Thread management: creation, lifecycle, sleep, delete, restart.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate internal comments into English.
 */

#include "start.h"

/* External scheduler structures */
extern s_list     s_thread_priority_table[START_THREAD_PRIORITY_MAX];
extern s_uint32_t s_thread_ready_priority_group;
extern s_list     s_thread_defunct_list;

/**
 * @brief Low-level field initialization (no state / ready list insertion).
 */
void _s_thread_init(s_pthread thread,
                    void *entry,
                    void *stackaddr,
                    s_uint32_t stacksize,
                    s_int8_t priority,
                    s_uint32_t tick)
{
    s_list_init(&thread->tlist);

    thread->entry            = entry;
    thread->stackaddr        = stackaddr;
    thread->stacksize        = stacksize;
    thread->current_priority = priority;
    thread->init_priority    = priority;
    thread->number_mask      = 1UL << thread->current_priority;

    /* Prepare initial stacked context (PSP). */
    thread->psp = (void *)s_stack_init(entry,
                                       (void *)((char *)stackaddr + stacksize));

    thread->init_tick      = tick;
    thread->remaining_tick = tick;
}

/**
 * @brief Public initialization (allocates timer and sets INIT state).
 */
s_status s_thread_init(s_pthread thread,
                       void *entry,
                       void *stackaddr,
                       s_uint32_t stacksize,
                       s_int8_t priority,
                       s_uint32_t tick)
{
    if (thread == NULL || entry == NULL || stackaddr == NULL || stacksize == 0)
        return S_NULL;
    if (priority < 0 || priority >= START_THREAD_PRIORITY_MAX)
        return S_INVALID;
    if (tick == 0)
        return S_INVALID;

    _s_thread_init(thread, entry, stackaddr, stacksize, priority, tick);

    /* Initialize per-thread timer (sleep/timeouts). */
    if (s_timer_init(&(thread->timer), timeout_function, thread, tick) != S_OK)
    return S_ERR;

    thread->status = START_THREAD_INIT;
    return S_OK;
}

/**
 * @brief Transition from INIT to READY: insert into ready queue.
 */
s_status s_thread_startup(s_pthread thread)
{
    register s_uint32_t level;

    if (thread == NULL)
        return S_NULL;
    if (thread->status == START_THREAD_DELETED)
        return S_ERR;

    level = s_irq_disable();

    thread->current_priority = thread->init_priority;
    thread->status           = START_THREAD_READY;
    thread->remaining_tick   = thread->init_tick;

    s_thread_ready_priority_group |= thread->number_mask;
    s_list_insert_before(&(s_thread_priority_table[thread->current_priority]),
                         &(thread->tlist));

    s_irq_enable(level);
    return S_OK;
}

/**
 * @brief Mark a thread TERMINATED (deferred reclamation by idle).
 */
s_status s_thread_delete(s_pthread thread)
{
    if (thread == NULL)
        return S_NULL;

    if (thread->status == START_THREAD_TERMINATED)
        return S_OK;
    if (thread->status == START_THREAD_DELETED)
        return S_ERR;

    s_sched_remove_thread(thread);
    s_timer_stop(&(thread->timer));

    thread->status = START_THREAD_TERMINATED;
    s_list_insert_before(&s_thread_defunct_list, &(thread->tlist));
    return S_OK;
}

/**
 * @brief Sleep current thread for specified ticks (blocking).
 */
void s_thread_sleep(s_uint32_t tick)
{
    s_pthread thread = s_thread_get();

    s_sched_remove_thread(thread);
    thread->status = START_THREAD_SUSPEND;

    s_timer_stop(&(thread->timer));
    s_timer_ctrl(&(thread->timer), START_TIMER_SET_TIME, &tick);
    s_timer_start(&(thread->timer));

    s_sched_switch();
}

/**
 * @brief Alias to s_thread_sleep().
 */
void s_delay(s_uint32_t tick)
{
    s_thread_sleep(tick);
}

/**
 * @brief Explicitly suspend a thread (not using timer).
 */
s_status s_thread_suspend(s_pthread thread)
{
    register s_uint32_t level;
    if (thread == NULL)
        return S_NULL;

    level = s_irq_disable();
    s_sched_remove_thread(thread);
    thread->status = START_THREAD_SUSPEND;
    s_irq_enable(level);
    return S_OK;
}

/**
 * @brief Generic control/query for thread properties.
 */
s_status s_thread_ctrl(s_pthread thread, s_uint32_t cmd, void *arg)
{
    if (thread == NULL)
        return S_NULL;

    switch (cmd)
    {
    case START_THREAD_GET_STATUS:
        if (arg) *(s_int32_t *)arg = thread->status;
        return S_OK;
    case START_THREAD_GET_PRIORITY:
        if (arg) *(s_uint8_t *)arg = thread->current_priority;
        return S_OK;
    case START_THREAD_SET_PRIORITY:
        if (arg)
        {
            thread->current_priority = *(s_uint8_t *)arg;
            thread->number_mask      = 1UL << thread->current_priority;
            return S_OK;
        }
        return S_ERR;
    default:
        return S_UNSUPPORTED;
    }
}

/**
 * @brief Reclaim all TERMINATED threads (move to DELETED).
 */
void s_cleanup_defunct_threads(void)
{
    register s_uint32_t level = s_irq_disable();
    while (!s_list_isempty(&s_thread_defunct_list))
    {
        s_pthread thread = S_LIST_ENTRY(s_thread_defunct_list.next,
                                        s_thread,
                                        tlist);
        thread->status = START_THREAD_DELETED;
        s_list_delete(&(thread->tlist));
    }
    s_irq_enable(level);
}

/**
 * @brief Restart a DELETED thread (reinitialize context & timer).
 */
s_status s_thread_restart(s_pthread thread)
{
    if (thread == NULL)
        return S_NULL;
    if (thread->status != START_THREAD_DELETED)
        return S_ERR;

    register s_uint32_t level = s_irq_disable();
    s_plist p = s_thread_defunct_list.next;
    while (p != &s_thread_defunct_list)
    {
        s_pthread t = S_LIST_ENTRY(p, s_thread, tlist);
        if (t == thread)
        {
            s_list_delete(&thread->tlist);
            break;
        }
        p = p->next;
    }
    s_irq_enable(level);

    _s_thread_init(thread,
                   thread->entry,
                   thread->stackaddr,
                   thread->stacksize,
                   thread->init_priority,
                   thread->timer.init_tick);

    s_timer_init(&(thread->timer), timeout_function, thread, thread->timer.init_tick);

    thread->status = START_THREAD_READY;
    return s_thread_startup(thread);
}

/**
 * @brief Terminate the current thread and trigger reschedule (never returns).
 */
void s_thread_exit(void)
{
    s_pthread t = s_thread_get();
    if (t == NULL)
        return;

    register s_uint32_t level = s_irq_disable();

    s_sched_remove_thread(t);
    s_timer_stop(&(t->timer));

    t->status = START_THREAD_TERMINATED;
    s_list_insert_before(&s_thread_defunct_list, &(t->tlist));

    s_irq_enable(level);

    s_sched_switch();

    for (;;)
    {
        /** Infinite loop safeguard: execution should not reach here if switch succeeds. */
    }
}

