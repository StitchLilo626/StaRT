/**
 * @file timer.c
 * @brief Timer management and system tick handling.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 Translate internal comments to English.
 */

#include "start.h"
#include <stddef.h>

/** Global monotonic tick counter (wraps on overflow). */
volatile s_uint32_t s_tick;

/** Timer skip-list levels (currently level count fixed by config). */
static s_list s_timer_list[START_TIMER_SKIP_LIST_LEVEL];

/**
 * @brief Initialize all timer list heads.
 */
void s_timer_list_init(void)
{
    int i;
    for (i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_init(&s_timer_list[i]);
    }
}

/**
 * @brief Convert milliseconds to system ticks.
 * @param ms Millisecond value.
 */
s_uint32_t s_tick_from_ms(s_uint32_t ms)
{
    if (ms == 0)
        return 0;
    return (ms * START_TICK) / 1000;
}

/**
 * @brief Busy-sleep current thread for millisecond duration.
 * @param ms Milliseconds to delay.
 */
void s_mdelay(s_uint32_t ms)
{
    s_uint32_t tick = s_tick_from_ms(ms);
    s_thread_sleep(tick);
}

/**
 * @brief Get current global tick count since system start.
 */
s_uint32_t s_tick_get(void)
{
    return s_tick;
}

/**
 * @brief Initialize a software timer object.
 * @param timer Timer control block.
 * @param timeout_func Callback invoked at expiration.
 * @param p User parameter for callback.
 * @param tick Initial duration in ticks.
 */
s_status s_timer_init(s_ptimer timer, void (*timeout_func)(void *p), void *p, s_uint32_t tick)
{
    if (timer == NULL || timeout_func == NULL)
        return S_NULL;

    /* Initialize all list level links to self. */
    for (int i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_init(&timer->row[i]);
    }

    timer->timeout_func = timeout_func;
    timer->p            = p;
    timer->init_tick    = tick;
    timer->timeout_tick = 0;

    return S_OK;
}

/**
 * @brief Tick ISR hook: increments global tick, manages time slice, checks timers.
 */
void s_tick_increase(void)
{
    register s_uint32_t level;
    s_pthread thread;

    ++s_tick;

    /* If scheduler not started yet, nothing else to process. */
    if (s_current_thread == NULL)
        return;

    thread = s_current_thread;

    /* Decrease remaining time slice atomically. */
    level = s_irq_disable();
    --thread->remaining_tick;
    if (thread->remaining_tick == 0)
    {
        thread->remaining_tick = thread->init_tick;
        s_irq_enable(level);
        s_thread_yield();
    }
    else
    {
        s_irq_enable(level);
    }

    /* Process timer expirations (callbacks executed outside critical section). */
    s_timer_check();
}

/**
 * @brief Remove timer from active list(s) if linked.
 */
s_inline void s_timer_remove(s_ptimer timer)
{
    register s_uint32_t level = s_irq_disable();
    for (int i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_delete(&timer->row[i]);
    }
    s_irq_enable(level);
}

/**
 * @brief Control timer (get/set duration).
 */
s_status s_timer_ctrl(s_ptimer timer, s_uint32_t cmd, void *arg)
{
    if (timer == NULL)
        return S_NULL;

    switch (cmd)
    {
    case START_TIMER_GET_TIME:
        if (arg) *(s_uint32_t *)arg = timer->init_tick;
        return S_OK;
    case START_TIMER_SET_TIME:
        timer->init_tick = *(s_uint32_t *)arg;
        return S_OK;
    default:
        return S_UNSUPPORTED;
    }
}

/**
 * @brief Stop a running timer (removes from list).
 */
s_status s_timer_stop(s_ptimer timer)
{
    if (timer == NULL)
        return S_NULL;

    s_timer_remove(timer);
    return S_OK;
}

/**
 * @brief Start (or restart) a software timer.
 * @param timer Timer object.
 */
s_status s_timer_start(s_ptimer timer)
{
    if (timer == NULL)
        return S_NULL;

    register s_uint32_t level = s_irq_disable();

    /* Remove in case already active to avoid duplicate nodes. */
    s_timer_remove(timer);

    /* Compute absolute expiration (handles wrap via signed diff on check). */
    timer->timeout_tick = s_tick_get() + timer->init_tick;

    /* Ordered insertion in level 0 list by timeout_tick. */
    s_plist p = &s_timer_list[0];
    while (p->next != &s_timer_list[0])
    {
        s_ptimer next_timer = S_LIST_ENTRY(p->next, s_timer, row[0]);
        if ((s_int32_t)(next_timer->timeout_tick - timer->timeout_tick) > 0)
            break;
        p = p->next;
    }
    s_list_insert_after(p, &timer->row[0]);

    s_irq_enable(level);
    return S_OK;
}

/**
 * @brief Scan active timers and invoke callbacks for expired timers.
 * @note Expired timers are first moved to a temp list to shorten IRQ-off window.
 */
void s_timer_check(void)
{
    register s_uint32_t level;
    s_list expired_list;

    s_list_init(&expired_list);

    level = s_irq_disable();
    while (!s_list_isempty(&s_timer_list[0]))
    {
        s_plist node  = s_timer_list[0].next;
        s_ptimer timer = S_LIST_ENTRY(node, s_timer, row[0]);

        if ((s_int32_t)(s_tick - timer->timeout_tick) >= 0)
        {
            s_list_delete(node);
            s_list_insert_before(&expired_list, node);
        }
        else
        {
            /* List ordered by timeout: stop when first non-expired. */
            break;
        }
    }
    s_irq_enable(level);

    /* Callbacks executed out of critical section to allow preemption. */
    while (!s_list_isempty(&expired_list))
    {
        s_plist node  = expired_list.next;
        s_ptimer timer = S_LIST_ENTRY(node, s_timer, row[0]);

        s_list_delete(node);

        if (timer->timeout_func)
            timer->timeout_func(timer->p);
    }
}

/**
 * @brief Default timeout callback used for thread sleep timers.
 * @param p Thread pointer.
 */
void timeout_function(void *p)
{
    s_pthread thread = (s_pthread)p;
    if (thread == NULL)
        return;

    thread->status = START_THREAD_READY;
    s_sched_insert_thread(thread);
    s_sched_switch();
}

