#include "start.h"
#include <stddef.h>

volatile s_uint32_t s_tick;

/* hard timer list */
static s_list s_timer_list[START_TIMER_SKIP_LIST_LEVEL];

void s_timer_list_init(void){
		int i = 0;
			 for (i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_init(&s_timer_list[i]);
    }
}

/**
 * @brief Convert milliseconds to system ticks.
 *
 * @param ms Milliseconds to convert.
 * @return The corresponding number of ticks.
 */
s_uint32_t s_tick_from_ms(s_uint32_t ms)
{
    if (ms == 0)
    {
        return 0;
    }
    return (ms * START_TICK) / 1000;
}

void s_mdelay(s_uint32_t ms)
{
    s_uint32_t tick = s_tick_from_ms(ms);
    s_thread_sleep(tick);
}


/**
 * This function will return curraent tick from operating system startup
 *
 * @return current tick
 */
s_uint32_t s_tick_get(void)
{
    /* return the global tick */
    return s_tick;
}

s_status s_timer_init(s_ptimer timer, void (*timeout_func)(void *p), void *p, s_uint32_t tick)
{

    if (timer == NULL || timeout_func == NULL)
        return S_NULL;

    // 初始化定时器的多级链表节点
    for (int i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_init(&timer->row[i]);
    }

    timer->timeout_func = timeout_func;
    timer->p = p; // 设置定时器参数
    timer->init_tick = tick;
    timer->timeout_tick = 0;

    return S_OK;
}

/**
 * This function will notify kernel there is one tick passed. Normally,
 * this function is invoked by clock ISR.
 */
void s_tick_increase(void)
{
    register s_uint32_t level;
    s_pthread thread;

		/* increase the global tick */
				++s_tick;		
	   if (s_current_thread == NULL)
    {
        // 调度器还未启动，直接返回
        return;
    }
    /* check time slice */
    thread = s_current_thread;

     level = rtos_irq_disable();

    --thread->remaining_tick;
    if (thread->remaining_tick == 0)
    {
        /* change to initialized tick */
        thread->remaining_tick = thread->init_tick;

        rtos_irq_enable(level);
        /* yield */
        s_thread_yield();
    }
     else
    {
        // 退出临界区
        rtos_irq_enable(level);
    }

    /* check timer */
    s_timer_check();
}

s_inline void s_timer_remove(s_ptimer timer)
{   
    register s_uint32_t level;

    level = rtos_irq_disable();

    // 从链表中删除定时器
    for (int i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
    {
        s_list_delete(&timer->row[i]);
    }
    rtos_irq_enable(level);
}

s_status s_timer_ctrl(s_ptimer timer, s_uint32_t cmd, void *arg)
{
    if (timer == NULL)
        return S_NULL;

    switch (cmd)
    {
    case START_TIMER_GET_TIME:
        arg = (void *)timer->init_tick;
        return S_OK;
    case START_TIMER_SET_TIME:
        timer->init_tick = *(s_uint32_t *)arg;
        return S_OK;
    default:
        return S_UNSUPPORTED;
    }
}

s_status s_timer_stop(s_ptimer timer)
{
    if (timer == NULL)
        return S_NULL;

    /*
    // 停止定时器，清除相关参数
    timer->timeout_tick = 0;
    timer->init_tick = 0;
    timer->timeout_func = NULL;
    timer->p = NULL;
    */
    s_timer_remove(timer);

    return S_OK;
}
/**
 * This function will start the timer
 *
 * @param timer the timer to be started
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
//s_status s_timer_start(s_ptimer timer)
//{
//    if (timer == NULL)
//        return S_NULL;
//    s_uint32_t level;
//     // 进入临界区
//    level = rtos_irq_disable();
//    // 先从所有层链表中移除，防止重复插入
//    s_timer_remove(timer);

//    // 设置超时时刻
//    timer->timeout_tick = s_tick_get() + timer->init_tick;

//    s_plist row_head[START_TIMER_SKIP_LIST_LEVEL];
//    int row_lvl;

//    // 初始化每层的头指针
//    for (row_lvl = 0; row_lvl < START_TIMER_SKIP_LIST_LEVEL; row_lvl++)
//        row_head[row_lvl] = &s_timer_list[row_lvl];

//    // 从最高层到最低层，找到插入点
//    for (row_lvl = START_TIMER_SKIP_LIST_LEVEL - 1; row_lvl >= 0; row_lvl--)
//    {
//        s_plist p = row_head[row_lvl];
//        while (p->next != &s_timer_list[row_lvl])
//        {
//            s_ptimer t = s_list_entry(p->next, s_timer, row[row_lvl]);
//            s_int32_t dt = t->timeout_tick - timer->timeout_tick;
//            if (dt == 0)
//            {
//                // 相同超时时间，继续往后找，插到同一超时时间节点后面
//                p = p->next;
//                continue;
//            }
//            else if ((s_uint32_t)dt < (1UL << 31)) // t->timeout_tick < timer->timeout_tick
//            {
//                break;
//            }
//            p = p->next;
//        }
//        // 在p后插入
//        s_list_insert_after(p, &timer->row[row_lvl]);
//        // 下一层从本层插入点开始
//        if (row_lvl > 0)
//            row_head[row_lvl - 1] = &timer->row[row_lvl];
//    }
//    rtos_irq_enable(level); // 恢复中断
//    return S_OK;
//}

s_status s_timer_start(s_ptimer timer)
{
    if (timer == NULL)
        return S_NULL;

    s_uint32_t level;
    level = rtos_irq_disable();

    // 1. 先从链表中移除，防止重复插入
    s_timer_remove(timer);

    // 2. 设置绝对超时时刻
    timer->timeout_tick = s_tick_get() + timer->init_tick;

    // 3. 寻找插入位置
    s_plist p = &s_timer_list[0]; // 获取链表头
    while (p->next != &s_timer_list[0])
    {
        // 获取下一个节点对应的 timer 结构体
        s_ptimer next_timer = s_list_entry(p->next, s_timer, row[0]);

        // 使用有符号数比较，处理 s_tick 回绕问题
        // 如果新定时器的超时时间比链表中的下一个定时器早，则找到了插入点
        if ((s_int32_t)(next_timer->timeout_tick - timer->timeout_tick) > 0)
        {
            break;
        }
        // 否则，继续向后找
        p = p->next;
    }

    // 4. 在找到的节点 p 之后插入新定时器
    // 此时 p 是新定时器在链表中的前一个节点
    s_list_insert_after(p, &timer->row[0]);

    rtos_irq_enable(level);
    return S_OK;
}

/**
 * This function will check the timer list, and call the timeout function
 * if the timer is timeout.
 */
//void s_timer_check(void)
//{
//    s_plist list_head = &s_timer_list[START_TIMER_SKIP_LIST_LEVEL - 1];

//    
//    // 进入临界区
//    s_uint32_t level = rtos_irq_disable();
//    while (!s_list_isempty(&s_timer_list[START_TIMER_SKIP_LIST_LEVEL - 1]))
//    {
//        s_plist node = list_head->next;
//        if (node == list_head)
//            break;

//        s_ptimer timer = s_list_entry(node, s_timer, row[START_TIMER_SKIP_LIST_LEVEL - 1]);
//        // 判断是否超时
//        if ((s_tick - timer->timeout_tick) < (1UL << 31))
//        {
//            // 超时，先保存下一个节点
//            s_plist next = node->next;
//            // 移除所有层
//            for (int i = 0; i < START_TIMER_SKIP_LIST_LEVEL; i++)
//                s_list_delete(&timer->row[i]);
//            // 调用回调
//            if (timer->timeout_func)
//                timer->timeout_func(timer->p);
//            // 继续检查下一个节点
//            continue;
//        }
//        else // 因为有序，后面都未超时，直接退出
//        {
//            break;
//        }
//    }
//    rtos_irq_enable(level); // 恢复中断
//}

void s_timer_check(void)
{
    s_uint32_t level;
    s_list expired_list; // 用于存放已到期定时器的临时链表

    s_list_init(&expired_list);

    level = rtos_irq_disable();

    // 1. 将所有已到期的定时器从主链表移动到临时链表
    while (!s_list_isempty(&s_timer_list[0]))
    {
        s_plist node = s_timer_list[0].next;
        s_ptimer timer = s_list_entry(node, s_timer, row[0]);

        // 判断是否超时
        if ((s_int32_t)(s_tick - timer->timeout_tick) >= 0)
        {
            // 已超时，从主链表移除
            s_list_delete(node);
            // 添加到临时链表的末尾
            s_list_insert_before(&expired_list, node);
        }
        else
        {
            // 因为链表有序，第一个未超时的节点之后都不会超时
            break;
        }
    }

    rtos_irq_enable(level);

    // 2. 遍历临时链表，执行所有到期定时器的回调函数
    // 这个过程在临界区之外，允许回调函数中发生抢占或启动新定时器
    while (!s_list_isempty(&expired_list))
    {
        s_plist node = expired_list.next;
        s_ptimer timer = s_list_entry(node, s_timer, row[0]);

        // 从临时链表中移除
        s_list_delete(node);

        // 执行回调
        if (timer->timeout_func)
        {
            timer->timeout_func(timer->p);
        }
    }
}

void timeout_function(void *p)
{
    register s_uint32_t level;
    s_pthread thread = (s_pthread)p;
    if (thread == NULL)
    {
        return; // 空指针错误
    }
    s_list_delete(&thread->tlist); // 从就绪队列中删除线程

    level = rtos_irq_disable();

    s_sched_insert_thread(thread);

    rtos_irq_enable(level);

    s_sched_switch();
}
