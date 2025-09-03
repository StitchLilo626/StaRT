/**
 * @file ipc.c
 * @brief IPC primitives: semaphore, mutex, message queue and suspend helpers.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate Chinese comments to English.
 */

#include "start.h"

#if START_USING_IPC

/**
 * @brief Suspend a thread into an IPC wait list (FIFO or PRIO).
 * @param list Suspend list head (sentinel).
 * @param thread Thread to suspend.
 * @param flag START_IPC_FLAG_FIFO or START_IPC_FLAG_PRIO.
 */
s_status s_ipc_suspend(s_list *list, s_pthread thread, s_uint8_t flag)
{
    register s_uint32_t level;
    s_plist p;

    if (list == NULL || thread == NULL)
        return S_NULL;

    /* enter critical */
    level = s_irq_disable();

    /* remove from ready queue (if any) and mark blocked */
    s_sched_remove_thread(thread);
    thread->status = START_THREAD_SUSPEND;

    /* insert into suspend list according to flag */
    switch (flag)
    {
    case START_IPC_FLAG_FIFO:
        s_list_insert_before(list, &thread->tlist);
        break;
    case START_IPC_FLAG_PRIO:/* PRIO (higher priority value -> lower priority number) */
         p = list->next;
        while (p != list)
        {
            s_pthread sth = S_LIST_ENTRY(p, s_thread, tlist);
            if (thread->current_priority < sth->current_priority)
            {
                s_list_insert_before(&sth->tlist, &thread->tlist);
                break;
            }
            p = p->next;
        }
        if (p == list)
        {
            /* not found position, append to end */
            s_list_insert_before(list, &thread->tlist);
        }
        break;
    default:
        /* Unsupported flag -> append FIFO style */
        s_list_insert_before(list, &thread->tlist);
        break;
    }

    s_irq_enable(level);
    return S_OK;
}

/**
 * @brief Resume all threads in given suspend list (no immediate schedule).
 * @param list Suspend list head.
 * @note Caller may invoke s_sched_switch() if desired.
 */
s_status s_ipc_list_resume_all(s_list *list)
{
    register s_uint32_t level;
    s_pthread thread;

    if (list == NULL)
        return S_NULL;

    while (!s_list_isempty(list))
    {
        level = s_irq_disable();
        thread = S_LIST_ENTRY(list->next, s_thread, tlist);
        s_list_delete(&thread->tlist);
        thread->status = START_THREAD_READY;
        s_sched_insert_thread(thread);
        s_irq_enable(level);
    }
    return S_OK;
}

#if START_USING_SEMAPHORE
/**
 * @brief Initialize semaphore.
 */
s_status s_sem_init(s_psem sem, s_uint16_t value, s_uint8_t flag)
{
    if (sem == NULL)
        return S_NULL;

    s_list_init(&sem->parent.suspend_thread);
    sem->count          = value;
    sem->reserved       = 0;
    sem->parent.flag    = flag;
    sem->parent.status  = 1;
    return S_OK;
}

/**
 * @brief Delete semaphore (resume all waiters).
 */
s_status s_sem_delete(s_psem sem)
{
    s_uint8_t need_schedule = 0;
    if (sem == NULL)
        return S_NULL;

    if (!s_list_isempty(&sem->parent.suspend_thread))
    {
        s_ipc_list_resume_all(&sem->parent.suspend_thread);
        need_schedule = 1;
    }

    sem->count         = 0;
    sem->reserved      = 0;
    sem->parent.status = 0;
    sem->parent.flag   = 0;

    if (need_schedule)
        s_sched_switch();
    return S_OK;
}

/**
 * @brief Acquire semaphore (with optional timeout).
 * @param time 0 = no wait, <0 wait forever, >0 tick timeout.
 */
s_status s_sem_take(s_psem sem, s_int32_t time)
{
    register s_uint32_t level;
    s_pthread thread;

    if (sem == NULL)
        return S_NULL;
    if (sem->parent.status == 0)
        return S_DELETED;

    level = s_irq_disable();
    if (sem->count > 0)
    {
        sem->count--;
        s_irq_enable(level);
        return S_OK;
    }
    if (time == 0)
    {
        s_irq_enable(level);
        return S_ERR;
    }

    thread = s_thread_get();
    if (thread == NULL)
    {
        s_irq_enable(level);
        return S_UNSUPPORTED;
    }

    s_ipc_suspend(&sem->parent.suspend_thread, thread, sem->parent.flag);

    if (time > 0)
    {
        s_timer_ctrl(&(thread->timer), START_TIMER_SET_TIME, &time);
        s_timer_start(&(thread->timer));
    }

    s_irq_enable(level);
    s_sched_switch();

    level = s_irq_disable();
    if (sem->count > 0)
    {
        sem->count--;
        s_irq_enable(level);
        return S_OK;
    }
    s_irq_enable(level);
    return S_ERR;
}

/**
 * @brief Release semaphore (wake one waiter if present).
 */
s_status s_sem_release(s_psem sem)
{
    register s_uint32_t level;
    s_pthread thread;
    s_uint8_t need_schedule = 0;

    if (sem == NULL)
        return S_NULL;
    if (sem->parent.status == 0)
        return S_DELETED;

    level = s_irq_disable();
    if (!s_list_isempty(&sem->parent.suspend_thread))
    {
        if (sem->count < SEM_VALUE_MAX)
            sem->count++;
        else
        {
            s_irq_enable(level);
            return S_ERR;
        }

        thread = S_LIST_ENTRY(sem->parent.suspend_thread.next, s_thread, tlist);
        s_list_delete(&thread->tlist);
        thread->status = START_THREAD_READY;
        s_sched_insert_thread(thread);
        need_schedule = 1;
    }
    else
    {
        if (sem->count < SEM_VALUE_MAX)
            sem->count++;
        else
        {
            s_irq_enable(level);
            return S_ERR;
        }
    }
    s_irq_enable(level);

    if (need_schedule)
        s_sched_switch();
    return S_OK;
}
#endif /* START_USING_SEMAPHORE */

#if START_USING_MUTEX
/**
 * @brief Initialize mutex (recursive + priority inheritance).
 */
s_status s_mutex_init(s_pmutex m, s_uint8_t flag)
{
    if (m == NULL) return S_NULL;

    s_list_init(&m->parent.suspend_thread);
    m->parent.flag   = flag;
    m->parent.status = 1;

    m->owner             = NULL;
    m->count             = 1;
    m->original_priority = 0xFF;
    m->hold              = 0;
    return S_OK;
}

/**
 * @brief Delete mutex, resume waiters and restore inherited priority.
 */
s_status s_mutex_delete(s_pmutex m)
{
    s_uint8_t need_schedule = 0;
    if (m == NULL) return S_NULL;
    if (m->parent.status == 0) return S_OK;

    if (!s_list_isempty(&m->parent.suspend_thread))
    {
        s_ipc_list_resume_all(&m->parent.suspend_thread);
        need_schedule = 1;
    }

    if (m->owner && m->original_priority != 0xFF &&
        m->owner->current_priority != m->original_priority)
    {
        s_thread_ctrl(m->owner, START_THREAD_SET_PRIORITY, &m->original_priority);
    }

    m->owner             = NULL;
    m->count             = 0;
    m->hold              = 0;
    m->original_priority = 0xFF;
    m->parent.status     = 0;
    m->parent.flag       = 0;

    if (need_schedule) s_sched_switch();
    return S_OK;
}

/**
 * @brief Acquire mutex (supports recursion and priority inheritance).
 */
s_status s_mutex_take(s_pmutex m, s_int32_t time)
{
    register s_uint32_t level;
    s_pthread self;

    if (m == NULL) return S_NULL;
    if (m->parent.status == 0) return S_DELETED;

    while (1)
    {
        level = s_irq_disable();
        if (m->parent.status == 0)
        {
            s_irq_enable(level);
            return S_DELETED;
        }

        self = s_thread_get();
        if (self == NULL)
        {
            s_irq_enable(level);
            return S_UNSUPPORTED;
        }

        if (m->owner == self)
        {
            if (m->hold < MUTEX_HOLD_MAX)
            {
                m->hold++;
                s_irq_enable(level);
                return S_OK;
            }
            s_irq_enable(level);
            return S_ERR;
        }

        if (m->count > 0)
        {
            m->count--;
            m->owner             = self;
            m->hold              = 1;
            m->original_priority = self->current_priority;
            s_irq_enable(level);
            return S_OK;
        }

        if (time == 0)
        {
            s_irq_enable(level);
            return S_ERR;
        }

        if (m->owner && self->current_priority < m->owner->current_priority)
        {
            if (m->original_priority == 0xFF)
                m->original_priority = m->owner->current_priority;
            s_thread_ctrl(m->owner,
                          START_THREAD_SET_PRIORITY,
                          &self->current_priority);
        }

        s_ipc_suspend(&m->parent.suspend_thread, self, m->parent.flag);

        if (time > 0)
        {
            s_timer_ctrl(&self->timer, START_TIMER_SET_TIME, &time);
            s_timer_start(&self->timer);
        }

        s_irq_enable(level);
        s_sched_switch();

        if (m->parent.status == 0)
            return S_DELETED;
        if (m->owner == self)
            return S_OK;

        if (time > 0)
        {
            level = s_irq_disable();
            if (m->owner != self && m->count == 0)
            {
                s_irq_enable(level);
                return S_ERR;
            }
            s_irq_enable(level);
        }
    }
}

/**
 * @brief Release mutex (handover or free, restore priority if needed).
 */
s_status s_mutex_release(s_pmutex m)
{
    register s_uint32_t level;
    s_uint8_t need_schedule = 0;
    s_pthread self;

    if (m == NULL) return S_NULL;
    if (m->parent.status == 0) return S_DELETED;

    level = s_irq_disable();
    self = s_thread_get();
    if (self != m->owner)
    {
        s_irq_enable(level);
        return S_ERR;
    }

    if (m->hold > 0)
        m->hold--;

    if (m->hold > 0)
    {
        s_irq_enable(level);
        return S_OK;
    }

    if (!s_list_isempty(&m->parent.suspend_thread))
    {
        s_pthread th = S_LIST_ENTRY(m->parent.suspend_thread.next, s_thread, tlist);
        s_list_delete(&th->tlist);

        if (m->original_priority != 0xFF &&
            self->current_priority != m->original_priority)
        {
            s_thread_ctrl(self, START_THREAD_SET_PRIORITY, &m->original_priority);
        }

        m->owner             = th;
        m->hold              = 1;
        m->count             = 0;
        m->original_priority = th->current_priority;

        th->status = START_THREAD_READY;
        s_sched_insert_thread(th);
        need_schedule = 1;
    }
    else
    {
        if (m->original_priority != 0xFF &&
            self->current_priority != m->original_priority)
        {
            s_thread_ctrl(self, START_THREAD_SET_PRIORITY, &m->original_priority);
        }
        m->owner             = NULL;
        m->original_priority = 0xFF;
        if (m->count < 1)
            m->count++;
    }

    s_irq_enable(level);

    if (need_schedule)
        s_sched_switch();
    return S_OK;
}
#endif /* START_USING_MUTEX */

#if START_USING_MESSAGEQUEUE
/**
 * @brief Initialize message queue with external memory pool.
 */
s_status s_msgqueue_init(s_pmsgqueue mq,
                         void *msg_pool,
                         s_uint16_t msg_size,
                         s_uint16_t pool_size,
                         s_uint8_t flag)
{
    if (mq == NULL || msg_pool == NULL)
        return S_NULL;
    if (msg_size == 0 || pool_size == 0)
        return S_INVALID;

    s_uint16_t align        = sizeof(s_uint32_t);
    s_uint16_t aligned_size = (s_uint16_t)((msg_size + (align - 1)) & ~(align - 1));
    s_uint16_t header_size  = (s_uint16_t)sizeof(struct s_mq_message);

    if (pool_size < (header_size + aligned_size))
        return S_INVALID;

    s_uint16_t max_msgs = (s_uint16_t)(pool_size / (header_size + aligned_size));
    if (max_msgs == 0)
        return S_INVALID;

    mq->parent.flag   = flag;
    mq->parent.status = 1;

    s_list_init(&mq->parent.suspend_thread);
    s_list_init(&mq->suspend_sender_thread);

    mq->msg_pool       = msg_pool;
    mq->msg_size       = aligned_size;
    mq->max_msgs       = max_msgs;
    mq->index          = 0;
    mq->msg_queue_head = NULL;
    mq->msg_queue_tail = NULL;
    mq->msg_queue_free = NULL;

    /* Build single-linked free list. */
    s_uint8_t *base = (s_uint8_t *)msg_pool;
    for (s_uint16_t i = 0; i < max_msgs; i++)
    {
        struct s_mq_message *node =
            (struct s_mq_message *)(base + i * (header_size + aligned_size));
        node->next       = (struct s_mq_message *)mq->msg_queue_free;
        mq->msg_queue_free = node;
    }
    return S_OK;
}

/**
 * @brief Delete message queue (resume all waiters and clear internal fields).
 */
s_status s_msgqueue_delete(s_pmsgqueue mq)
{
    s_uint8_t need_schedule = 0;

    if (mq == NULL)
        return S_NULL;

    if (!s_list_isempty(&mq->parent.suspend_thread))
    {
        s_ipc_list_resume_all(&mq->parent.suspend_thread);
        need_schedule = 1;
    }
    if (!s_list_isempty(&mq->suspend_sender_thread))
    {
        s_ipc_list_resume_all(&mq->suspend_sender_thread);
        need_schedule = 1;
    }

    mq->msg_queue_head = NULL;
    mq->msg_queue_tail = NULL;
    mq->msg_queue_free = NULL;
    mq->msg_pool       = NULL;
    mq->msg_size       = 0;
    mq->max_msgs       = 0;
    mq->index          = 0;

    mq->parent.status = 0;
    mq->parent.flag   = 0;

    if (need_schedule)
        s_sched_switch();
    return S_OK;
}

/* Internal copy helper */
static void __s_msg_copy_out(s_uint8_t *dst, const s_uint8_t *src, s_uint16_t len)
{
    while (len--) *dst++ = *src++;
}

/**
 * @brief Send message with optional blocking when full.
 */
s_status s_msgqueue_send_wait(s_pmsgqueue mq,
                              const void *buffer,
                              s_uint16_t size,
                              s_int32_t timeout)
{
    register s_uint32_t level;
    struct s_mq_message *node;
    s_pthread thread;
    s_uint32_t start_tick = 0;

    if (mq == NULL || buffer == NULL)
        return S_NULL;
    if (mq->parent.status == 0)
        return S_DELETED;
    if (size == 0 || size > mq->msg_size)
        return S_INVALID;

    while (1)
    {
        level = s_irq_disable();
        if (mq->parent.status == 0)
        {
            s_irq_enable(level);
            return S_DELETED;
        }

        if (mq->msg_queue_free != NULL)
        {
            node = (struct s_mq_message *)mq->msg_queue_free;
            mq->msg_queue_free = node->next;
            s_irq_enable(level);

            __s_msg_copy_out((s_uint8_t *)(node + 1), (const s_uint8_t *)buffer, size);

            level = s_irq_disable();
            node->next = NULL;
            if (mq->msg_queue_tail)
                ((struct s_mq_message *)mq->msg_queue_tail)->next = node;
            mq->msg_queue_tail = node;
            if (mq->msg_queue_head == NULL)
                mq->msg_queue_head = node;
            mq->index++;

            if (!s_list_isempty(&mq->parent.suspend_thread))
            {
                s_pthread rth = S_LIST_ENTRY(mq->parent.suspend_thread.next, s_thread, tlist);
                s_list_delete(&rth->tlist);
                rth->status = START_THREAD_READY;
                s_sched_insert_thread(rth);
                s_irq_enable(level);
                s_sched_switch();
            }
            else
            {
                s_irq_enable(level);
            }
            return S_OK;
        }

        if (timeout == 0)
        {
            s_irq_enable(level);
            return S_ERR;
        }

        thread = s_thread_get();
        if (thread == NULL)
        {
            s_irq_enable(level);
            return S_UNSUPPORTED;
        }

        s_ipc_suspend(&mq->suspend_sender_thread, thread, mq->parent.flag);

        if (timeout > 0)
        {
            if (start_tick == 0)
                start_tick = s_tick_get();
            s_timer_ctrl(&thread->timer, START_TIMER_SET_TIME, &timeout);
            s_timer_start(&thread->timer);
        }

        s_irq_enable(level);
        s_sched_switch();

        if (mq->parent.status == 0)
            return S_DELETED;

        if (timeout > 0)
        {
            s_uint32_t now     = s_tick_get();
            s_uint32_t elapsed = now - start_tick;
            if ((s_int32_t)elapsed >= timeout)
                return S_ERR;
            timeout    -= (s_int32_t)elapsed;
            start_tick  = now;
        }
    }
}

/**
 * @brief Non-blocking send wrapper.
 */
s_status s_msgqueue_send(s_pmsgqueue mq,
                         const void *buffer,
                         s_uint16_t size)
{
    return s_msgqueue_send_wait(mq, buffer, size, 0);
}

/**
 * @brief Urgent send (insert at head, non-blocking).
 */
s_status s_msgqueue_urgent(s_pmsgqueue mq,
                           const void *buffer,
                           s_uint16_t size)
{
    register s_uint32_t level;
    struct s_mq_message *node;

    if (mq == NULL || buffer == NULL)
        return S_NULL;
    if (mq->parent.status == 0)
        return S_DELETED;
    if (size == 0 || size > mq->msg_size)
        return S_INVALID;

    level = s_irq_disable();
    if (mq->msg_queue_free == NULL)
    {
        s_irq_enable(level);
        return S_ERR; /* 满 */
    }
    node = (struct s_mq_message *)mq->msg_queue_free;
    mq->msg_queue_free = node->next;
    s_irq_enable(level);

    __s_msg_copy_out((s_uint8_t *)(node + 1), (const s_uint8_t *)buffer, size);

    level = s_irq_disable();
    node->next        = (struct s_mq_message *)mq->msg_queue_head;
    mq->msg_queue_head = node;
    if (mq->msg_queue_tail == NULL)
        mq->msg_queue_tail = node;
    mq->index++;

    if (!s_list_isempty(&mq->parent.suspend_thread))
    {
        s_pthread rth = S_LIST_ENTRY(mq->parent.suspend_thread.next, s_thread, tlist);
        s_list_delete(&rth->tlist);
        rth->status = START_THREAD_READY;
        s_sched_insert_thread(rth);
        s_irq_enable(level);
        s_sched_switch();
    }
    else
    {
        s_irq_enable(level);
    }
    return S_OK;
}

/**
 * @brief Receive message with optional blocking.
 */
s_status s_msgqueue_recv(s_pmsgqueue mq,
                         void *buffer,
                         s_uint16_t size,
                         s_int32_t timeout)
{
    register s_uint32_t level;
    struct s_mq_message *node;
    s_pthread thread;
    s_uint32_t start_tick = 0;

    if (mq == NULL || buffer == NULL)
        return S_NULL;
    if (mq->parent.status == 0)
        return S_DELETED;
    if (size == 0)
        return S_INVALID;

    while (1)
    {
        level = s_irq_disable();
        if (mq->parent.status == 0)
        {
            s_irq_enable(level);
            return S_DELETED;
        }

        if (mq->msg_queue_head != NULL && mq->index > 0)
        {
            node = (struct s_mq_message *)mq->msg_queue_head;
            mq->msg_queue_head = node->next;
            if (mq->msg_queue_tail == node)
                mq->msg_queue_tail = NULL;
            mq->index--;
            s_irq_enable(level);

            s_uint16_t copy_len = size > mq->msg_size ? mq->msg_size : size;
            __s_msg_copy_out((s_uint8_t *)buffer, (s_uint8_t *)(node + 1), copy_len);

            level = s_irq_disable();
            node->next        = (struct s_mq_message *)mq->msg_queue_free;
            mq->msg_queue_free = node;

            if (!s_list_isempty(&mq->suspend_sender_thread))
            {
                s_pthread sth = S_LIST_ENTRY(mq->suspend_sender_thread.next, s_thread, tlist);
                s_list_delete(&sth->tlist);
                sth->status = START_THREAD_READY;
                s_sched_insert_thread(sth);
                s_irq_enable(level);
                s_sched_switch();
            }
            else
            {
                s_irq_enable(level);
            }
            return S_OK;
        }

        if (timeout == 0)
        {
            s_irq_enable(level);
            return S_ERR;
        }

        thread = s_thread_get();
        if (thread == NULL)
        {
            s_irq_enable(level);
            return S_UNSUPPORTED;
        }

        s_ipc_suspend(&mq->parent.suspend_thread, thread, mq->parent.flag);
        if (timeout > 0)
        {
            if (start_tick == 0)
                start_tick = s_tick_get();
            s_timer_ctrl(&thread->timer, START_TIMER_SET_TIME, &timeout);
            s_timer_start(&thread->timer);
        }

        s_irq_enable(level);
        s_sched_switch();

        if (mq->parent.status == 0)
            return S_DELETED;

        if (timeout > 0)
        {
            s_uint32_t now     = s_tick_get();
            s_uint32_t elapsed = now - start_tick;
            if ((s_int32_t)elapsed >= timeout)
                return S_ERR;
            timeout    -= (s_int32_t)elapsed;
            start_tick  = now;
        }
    }
}
#endif /* START_USING_MESSAGEQUEUE */

#endif /* START_USING_IPC */
