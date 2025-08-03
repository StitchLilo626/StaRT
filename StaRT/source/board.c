#include "start.h"

// 暂时放在这里
//  静态分配idle线程对象和栈
int idle = 0;
static s_thread idle_thread;
static s_uint8_t idle_stack[512]; // idle线程栈大小可根据需求调整

// idle线程入口函数
static void idle_thread_entry(void)
{

    while (1)
    {
        idle++; // 可以在这里执行省电操作或WFI指令
        //__asm volatile ("wfi");
    }
}

s_status s_idle_thread_init(void)
{
    // 初始化idle线程，优先级最低
    s_thread_init(&idle_thread,
                  idle_thread_entry,
                  idle_stack,
                  sizeof(idle_stack),
                  START_THREAD_PRIORITY_MAX - 1, // 最低优先级
                  5);
    s_thread_startup(&idle_thread);

    return S_OK;
}
s_status s_start_init(void){
    
    s_sched_init();
    s_timer_list_init();
    s_idle_thread_init(); // 初始化idle线程
    return S_OK;
}