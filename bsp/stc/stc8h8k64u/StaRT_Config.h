#ifndef START_STC8H_CONFIG_H
#define START_STC8H_CONFIG_H

#define START_VERSION                   "1.0.2"
#define START_THREAD_PRIORITY_MAX       8
#define START_USING_CPU_FFS             1
#define START_TIMER_SKIP_LIST_LEVEL     1
#define START_TICK                      100
#define S_PRINTF_BUF_SIZE               48
#define START_STC8H_STACK_IMAGE_SIZE    256
#define START_IDLE_STACK_SIZE           START_STC8H_STACK_IMAGE_SIZE
#define START_USING_IPC                 1
#define START_USING_SEMAPHORE           1
#define START_USING_MUTEX               0
#define START_USING_MESSAGEQUEUE        0
#define START_DEBUG                     0
#define START_USING_UDELAY              0
#define START_IDLE_HOOK_LIST_SIZE       0

#endif
