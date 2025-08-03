#include "sdef.h"
#include "start.h"

s_pthread s_current_thread;      /*��ǰ�߳�*/
s_uint8_t rtos_current_priority; /*��ǰ�߳����ȼ�*/

s_uint32_t rtos_prev_thread_sp_p; /*��һ���߳�sp��ָ��*/
s_uint32_t rtos_next_thread_sp_p; /*��һ���߳�sp��ָ��*/
s_uint32_t rtos_interrupt_flag;   /*PendSV�жϱ�־*/

s_list s_thread_priority_table[START_THREAD_PRIORITY_MAX]; /*�߳����ȼ���*/
s_uint32_t s_thread_ready_priority_group = 0;              /*�����߳����ȼ���*/

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

    // ����������ȼ�
    highest_ready_priority = __s_ffs(s_thread_ready_priority_group) - 1;
    if (highest_ready_priority >= START_THREAD_PRIORITY_MAX)
        return; // û�пɵ����߳�

    // ��ȡ������ȼ������еĵ�һ���߳�
    next_thread = s_list_entry(s_thread_priority_table[highest_ready_priority].next, s_thread, tlist);

    // �����ǰ�߳̾���������ȼ��̣߳����л�
    if (s_current_thread == next_thread)
        return;

    prev_thread = s_current_thread;
    s_current_thread = next_thread;

    // �����߳��л�
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
    // �����ٽ���
    level = rtos_irq_disable();
    // �����ȼ��������Ƴ����߳�
    s_list_delete(&thread->tlist);

    // ������ȼ����еĶ�Ӧλ
    // ��������ȼ���û�������̣߳�������
    if (s_list_isempty(&s_thread_priority_table[thread->current_priority]))
    {
        s_thread_ready_priority_group &= ~(thread->number_mask);
    }

        // �˳��ٽ���
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
     // �����ٽ���
    level = rtos_irq_disable();

    // ���뵽��Ӧ���ȼ��ľ�������
    s_list_insert_before(&(s_thread_priority_table[thread->current_priority]), &(thread->tlist));

    // �������ȼ����Ӧλ
    s_thread_ready_priority_group |= thread->number_mask;

        // �˳��ٽ���
    rtos_irq_enable(level);
}

void s_thread_yield(void)
{
    register s_uint32_t level;
    s_pthread yield_thread = s_current_thread;

    s_plist priority_list = &s_thread_priority_table[yield_thread->current_priority];

    level = rtos_irq_disable();
    // ��������ȼ�����ֻ���Լ��������л�
    if (priority_list->next == &(yield_thread->tlist) && priority_list->prev == &(yield_thread->tlist))
        return;
    s_list_delete(&yield_thread->tlist);
    s_list_insert_before(&(s_thread_priority_table[yield_thread->current_priority]),
                         &(yield_thread->tlist));

    // �˳��ٽ���
    rtos_irq_enable(level);

    // �ó�CPU�����������������߳��л�
    s_sched_switch();
}

