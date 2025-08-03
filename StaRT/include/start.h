#ifndef __START_H_
#define __START_H_
#include "sdef.h"
#include "list.h"
#include "stdio.h"


extern s_pthread s_current_thread;          /*当前线程*/
extern s_uint8_t rtos_current_priority;     	/*当前线程优先级*/
// 线程切换相关（汇编实现）
// 普通线程切换，prev为上一个线程的psp指针，next为下一个线程的psp指针
void rtos_normal_switch_task(s_uint32_t prev, s_uint32_t next);

// 第一次线程切换，只需要切到next线程
void rtos_frist_switch_task(s_uint32_t next);

// 关中断，返回上一次中断状态
s_uint32_t rtos_irq_disable();

// 恢复中断，参数为上一次的中断状态
void rtos_irq_enable(s_uint32_t disirq);

// 线程栈初始化，entry为线程入口，stackaddr为栈顶地址，返回初始化后的栈指针
s_uint8_t *rtos_stack_init(void *entry, s_uint8_t *stackaddr);

s_pthread s_thread_get(void);

s_status s_start_init(void);

void s_sched_init(void);
// 启动调度器，开始多线程调度
void rtos_sched_start();

// 线程切换（由调度器调用）
void s_sched_switch();

void s_sched_remove_thread(s_pthread thread);

void s_sched_insert_thread(s_pthread thread);

void s_thread_yield(void);

void s_thread_sleep(s_uint32_t tick);

// 线程初始化，thread为线程对象，entry为入口函数，stackaddr为栈空间，stacksize为栈大小
void s_thread_init(s_pthread thread,
                      void *entry,
                      void *stackaddr,
                      s_uint32_t stacksize,
                     s_int8_t priority,
                      s_uint32_t tick);

s_status s_thread_startup(s_pthread thread);

void s_timer_list_init(void);
											
void s_mdelay(s_uint32_t ms);

void s_delay(s_uint32_t tick);

s_status s_timer_init(s_ptimer timer, void (*timeout_func)(void *p),void *p,s_uint32_t tick);

void s_timer_check(void);
											
void s_tick_increase(void);

s_status s_timer_ctrl(s_ptimer timer, s_uint32_t cmd, void *arg);

s_status s_timer_stop(s_ptimer timer);

s_status s_timer_start(s_ptimer timer);

void timeout_function (void *p);

int __rt_ffs(int value);

#endif


