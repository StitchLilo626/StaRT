#include "sdef.h"
#include "start.h"

s_pthread s_current_thread;      /*当前线程*/
s_uint8_t rtos_current_priority; /*当前线程优先级*/

s_uint32_t rtos_prev_thread_sp_p; /*上一个线程sp的指针*/
s_uint32_t rtos_next_thread_sp_p; /*下一个线程sp的指针*/
s_uint32_t rtos_interrupt_flag;   /*PendSV中断标志*/

s_list s_thread_priority_table[START_THREAD_PRIORITY_MAX]; /*线程优先级表*/
s_uint32_t s_thread_ready_priority_group = 0;              /*就绪线程优先级组*/

/**
 * This function will return current thread from operating system startup
 *
 * @return current thread
 */
s_pthread s_thread_get(void)
{
    /* return s_current_thread */
    return s_current_thread;
}





void s_sched_init(void)
{
    s_uint8_t i;
    for (i = 0; i < START_THREAD_PRIORITY_MAX; i++)
    {
        s_list_init(&s_thread_priority_table[i]);
    }
		
    s_current_thread = NULL;

}

void rtos_sched_start(void)
{
    register s_pthread next_thread;
    register s_uint32_t highest_ready_priority;

    highest_ready_priority = __s_ffs(s_thread_ready_priority_group) - 1;
    next_thread = s_list_entry(s_thread_priority_table[highest_ready_priority].next, s_thread, tlist);
    s_current_thread = next_thread;
    rtos_frist_switch_task((s_uint32_t)&next_thread->psp);
}

void s_sched_switch(void)
{
    register s_pthread next_thread;
    register s_pthread prev_thread;
    register s_uint32_t highest_ready_priority;

    // 查找最高优先级
    highest_ready_priority = __s_ffs(s_thread_ready_priority_group) - 1;
    if (highest_ready_priority >= START_THREAD_PRIORITY_MAX)
        return; // 没有可调度线程

    // 获取最高优先级队列中的第一个线程
    next_thread = s_list_entry(s_thread_priority_table[highest_ready_priority].next, s_thread, tlist);

    // 如果当前线程就是最高优先级线程，则不切换
    if (s_current_thread == next_thread)
        return;

    prev_thread = s_current_thread;
    s_current_thread = next_thread;

    // 进行线程切换
    rtos_normal_switch_task((s_uint32_t)&prev_thread->psp, (s_uint32_t)&next_thread->psp);
}

/*
 * This function will remove a thread from system ready queue.
 *
 * @param thread the thread to be removed
 *
 * @note Please do not invoke this function in user application.
 */
void s_sched_remove_thread(s_pthread thread)
{
    register s_uint32_t level;
    if (thread == NULL)
        return;
    // 进入临界区
    level = rtos_irq_disable();
    // 从优先级队列中移除该线程
    s_list_delete(&thread->tlist);

    // 清除优先级组中的对应位
    // 如果该优先级下没有其他线程，则清零
    if (s_list_isempty(&s_thread_priority_table[thread->current_priority]))
    {
        s_thread_ready_priority_group &= ~(thread->number_mask);
    }

        // 退出临界区
    rtos_irq_enable(level);
}

/*
 * This function will insert a thread into system ready queue.
 *
 * @param thread the thread to be inserted
 *
 * @note Please do not invoke this function in user application.
 */
void s_sched_insert_thread(s_pthread thread)
{
    register s_uint32_t level;

    if (thread == NULL)
        return;
     // 进入临界区
    level = rtos_irq_disable();

    // 插入到对应优先级的就绪队列
    s_list_insert_before(&(s_thread_priority_table[thread->current_priority]), &(thread->tlist));

    // 设置优先级组对应位
    s_thread_ready_priority_group |= thread->number_mask;

        // 退出临界区
    rtos_irq_enable(level);
}

void s_thread_yield(void)
{
    register s_uint32_t level;
    s_pthread yield_thread = s_current_thread;

    s_plist priority_list = &s_thread_priority_table[yield_thread->current_priority];

    level = rtos_irq_disable();
    // 如果本优先级队列只有自己，不必切换
    if (priority_list->next == &(yield_thread->tlist) && priority_list->prev == &(yield_thread->tlist))
        return;
    s_list_delete(&yield_thread->tlist);
    s_list_insert_before(&(s_thread_priority_table[yield_thread->current_priority]),
                         &(yield_thread->tlist));

    // 退出临界区
    rtos_irq_enable(level);

    // 让出CPU，触发调度器进行线程切换
    s_sched_switch();
}

