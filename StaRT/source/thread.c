#include "start.h"

extern s_list s_thread_priority_table[START_THREAD_PRIORITY_MAX];  /*线程优先级表*/
extern s_uint32_t s_thread_ready_priority_group; /*就绪线程优先级组*/

void _s_thread_init(s_pthread thread,
                      void *entry,
                      void *stackaddr,
                      s_uint32_t stacksize,
                      s_int8_t priority,
                      s_uint32_t tick)
{
    s_list_init(&thread->tlist); // 初始化线程链表节点
    thread->entry = (void *)entry;  // 设置线程入口函数
    thread->stackaddr = stackaddr;  // 设置线程栈空间起始地址
    thread->stacksize = stacksize;  // 设置线程栈大小
    thread->current_priority = priority; // 设置线程当前优先级
    thread->init_priority = priority; // 设置线程初始化优先级
    thread->number_mask = 1L << thread->current_priority;
    // 初始化线程栈，并保存初始栈指针
    thread->psp = (void *)rtos_stack_init(entry, (void *)((char *)stackaddr + stacksize));
}

void s_thread_init(s_pthread thread,
                      void *entry,
                      void *stackaddr,
                      s_uint32_t stacksize,
                      s_int8_t priority,
                      s_uint32_t tick){
    // 参数有效性检查
    if (thread == NULL || entry == NULL || stackaddr == NULL || stacksize == 0)
        return;
    if (priority < 0 || priority >= START_THREAD_PRIORITY_MAX)
        return;
    if (tick == 0)
        return;
    _s_thread_init(thread,entry,stackaddr,stacksize,priority,tick);
    s_timer_init(&(thread->timer),timeout_function,thread, tick);

}





 s_status s_thread_startup(s_pthread thread){

     register s_uint32_t level;
    if (thread == NULL) {
        return S_NULL; // 空指针错误
    }
    level = rtos_irq_disable();

    thread->current_priority = thread->init_priority;

    s_thread_ready_priority_group |= thread->number_mask;
    // 将线程添加到调度器的就绪队列中
    s_list_insert_before(&(s_thread_priority_table[thread->current_priority]), &(thread->tlist));

    rtos_irq_enable(level);
    return S_OK; // 成功
 }


void s_thread_sleep(s_uint32_t tick)
{   
    s_pthread thread;
    register s_uint32_t level = rtos_irq_disable();
    
    thread = s_thread_get();

    
    // 从就绪队列中移除线程
    s_sched_remove_thread(thread);

    // 停止定时器并设置超时时间
    s_timer_stop(&(thread->timer));
    s_timer_ctrl(&(thread->timer), START_TIMER_SET_TIME, &tick);
    
    // 启动定时器
    s_timer_start(&(thread->timer));
    
    rtos_irq_enable(level); // 恢复中断
    // 切换到下一个线程
    s_sched_switch();

}


/**
 * This function will let current thread delay for some ticks.
 *
 * @param tick the delay ticks
 *
 * @return none
 */
void s_delay(s_uint32_t tick)
{
    s_thread_sleep(tick);
}
