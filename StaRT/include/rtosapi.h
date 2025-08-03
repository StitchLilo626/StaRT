#ifndef __API_H_
#define __API_H_
#include "sdef.h"

// 初始化链表节点
void s_list_init(s_plist l);

// 在链表节点 l 之后插入节点 n
void s_list_insert_after(s_plist l,s_plist n);

// 在链表节点 l 之前插入节点 n
void s_list_insert_before(s_plist l,s_plist n);

// 删除链表节点 d
void s_list_delete(s_plist d);

// 判断链表是否为空（返回1为空，0为非空）
int s_list_isempty(s_plist l);

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

// 启动调度器，开始多线程调度
void rtos_sched_start();

// 线程切换（由调度器调用）
void s_sched_switch();

// 线程初始化，thread为线程对象，entry为入口函数，stackaddr为栈空间，stacksize为栈大小
void s_thread_init(s_pthread thread,
                      void *entry,
                      void *stackaddr,
                      s_uint32_t stacksize);

#endif

