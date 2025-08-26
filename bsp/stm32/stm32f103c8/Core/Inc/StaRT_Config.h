#ifndef __SCONFIG_H_
#define __SCONFIG_H_

/*1:开启资源，0:关闭资源*/

#define START_VERSION "1.0.2"

#define START_THREAD_PRIORITY_MAX      32
#define START_USING_CPU_FFS            1
#define START_TIMER_SKIP_LIST_LEVEL    1
#define START_TICK                     1000 // 每秒1000个tick

#define S_PRINTF_BUF_SIZE              128  // 定义缓冲区大小

#define START_IDLE_STACK_SIZE          256  // 定义空闲线程栈大小

#define START_USING_MUTEX               1
#define START_USING_SEMAPHORE           1
#define START_USING_MESSAGEQUEUE        1

#define START_DEBUG                     1
#define START_USING_IPC                 1







#endif


