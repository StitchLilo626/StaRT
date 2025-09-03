/**
 * @file scheduler.c
 * @brief Priority-based ready queue scheduler and context switching glue.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate in-code comments to English.
 */

#include "sdef.h"
#include "start.h"

/** Currently running thread pointer. */
s_pthread s_current_thread;
/** Currently running thread priority. */
s_uint8_t s_current_priority;

/** Storage for previous thread PSP pointer (used by assembly switch). */
s_uint32_t s_prev_thread_sp_p;
/** Storage for next thread PSP pointer (used by assembly switch). */
s_uint32_t s_next_thread_sp_p;
/** PendSV trigger flag (set before requesting context switch). */
s_uint32_t s_interrupt_flag;

/** Per-priority ready queues (circular list heads). */
s_list s_thread_priority_table[START_THREAD_PRIORITY_MAX];
/** Bitmask indicating which priorities have at least one ready thread. */
s_uint32_t s_thread_ready_priority_group = 0;
/** List of threads waiting final reclamation (TERMINATED ¡ú DELETED). */
s_list s_thread_defunct_list;

/**
 * @brief Get current running thread.
 */
s_pthread s_thread_get(void)
{
    return s_current_thread;
}

/**
 * @brief Initialize scheduler internal structures.
 */
void s_sched_init(void)
{
    s_uint8_t i;
    for (i = 0; i < START_THREAD_PRIORITY_MAX; i++)
    {
        s_list_init(&s_thread_priority_table[i]);
    }
    s_list_init(&s_thread_defunct_list);
    s_current_thread = NULL;
}

/**
 * @brief Start scheduling: select highest ready and perform first context switch.
 * @note Assumes at least one thread is ready.
 */
void s_sched_start(void)
{
    register s_pthread  next_thread;
    register s_uint32_t highest_ready_priority;

    highest_ready_priority = __s_ffs(s_thread_ready_priority_group) - 1;
    next_thread = S_LIST_ENTRY(
        s_thread_priority_table[highest_ready_priority].next,
        s_thread,
        tlist);

    /* Mark chosen thread as running and reload its time slice. */
    s_current_thread            = next_thread;
    s_current_priority          = next_thread->current_priority;
    next_thread->status         = START_THREAD_RUNNING;
    next_thread->remaining_tick = next_thread->init_tick;

    s_first_switch_task((s_uint32_t)&next_thread->psp);
}

/**
 * @brief Attempt a context switch to higher-priority ready thread (if any).
 */
void s_sched_switch(void)
{
    register s_pthread  next_thread;
    register s_pthread  prev_thread;
    register s_uint32_t highest_ready_priority;

    highest_ready_priority = __s_ffs(s_thread_ready_priority_group) - 1;
    if (highest_ready_priority >= START_THREAD_PRIORITY_MAX)
        return;

    next_thread = S_LIST_ENTRY(
        s_thread_priority_table[highest_ready_priority].next,
        s_thread,
        tlist);

    if (s_current_thread == next_thread)
        return;

    prev_thread      = s_current_thread;
    s_current_thread = next_thread;

    if (prev_thread && prev_thread->status == START_THREAD_RUNNING)
        prev_thread->status = START_THREAD_READY;

    next_thread->status = START_THREAD_RUNNING;
    s_current_priority  = next_thread->current_priority;

    s_normal_switch_task((s_uint32_t)&prev_thread->psp,
                         (s_uint32_t)&next_thread->psp);
}

/**
 * @brief Remove a thread from ready queue (and clear ready bit if empty).
 * @param thread Thread to be removed.
 * @note Must not be NULL.
 */
void s_sched_remove_thread(s_pthread thread)
{
    register s_uint32_t level;
    if (thread == NULL)
        return;

    level = s_irq_disable();

    s_list_delete(&thread->tlist);

    if (s_list_isempty(&s_thread_priority_table[thread->current_priority]))
    {
        s_thread_ready_priority_group &= ~(thread->number_mask);
    }

    s_irq_enable(level);
}

/**
 * @brief Insert a thread into ready queue and set ready bit.
 */
void s_sched_insert_thread(s_pthread thread)
{
    register s_uint32_t level;

    if (thread == NULL)
        return;

    level = s_irq_disable();

    s_list_insert_before(&(s_thread_priority_table[thread->current_priority]),
                         &(thread->tlist));
    s_thread_ready_priority_group |= thread->number_mask;

    s_irq_enable(level);
}

/**
 * @brief Voluntarily yield CPU within same priority group (round robin).
 */
void s_thread_yield(void)
{
    register s_uint32_t level;
    s_pthread yield_thread = s_thread_get();

    s_plist priority_list =
        &s_thread_priority_table[yield_thread->current_priority];

    level = s_irq_disable();

    /* If only one thread at this priority, no rotation needed. */
    if (priority_list->next == &(yield_thread->tlist) &&
        priority_list->prev == &(yield_thread->tlist))
    {
        s_irq_enable(level);
        return;
    }

    /* Move current thread to queue tail (before head sentinel). */
    s_list_delete(&yield_thread->tlist);
    s_list_insert_before(
        &(s_thread_priority_table[yield_thread->current_priority]),
        &(yield_thread->tlist));

    s_irq_enable(level);

    s_sched_switch();
}

