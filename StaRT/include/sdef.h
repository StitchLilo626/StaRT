#ifndef __TYPEDEF_H_
#define __TYPEDEF_H_
#include "StaRT_Config.h"

typedef char 			s_int8_t;
typedef unsigned char 	s_uint8_t;
typedef int 			s_int16_t;
typedef unsigned int 	s_uint16_t;
typedef long 			s_int32_t;
typedef unsigned long 	s_uint32_t;

typedef enum
{
    S_OK = 0,         // 操作成功
    S_ERR = -1,       // 通用错误
    S_TIMEOUT = -2,   // 超时
    S_BUSY = -3,      // 资源忙
    S_INVALID = -4,   // 参数无效
    S_NULL = -5,      // 空指针
    S_UNSUPPORTED = -6// 不支持的操作
} s_status;


typedef struct list
{
	struct list *next;
	struct list *prev;
}s_list,*s_plist;

/**
 * timer structure
 */
typedef struct timer
{

    s_list        row[START_TIMER_SKIP_LIST_LEVEL];

    void (*timeout_func)(void *p);              /**< timeout function */
	void *p;                          /**< timeout function parameter */

    s_uint32_t        init_tick;                         /**< timer timeout tick */
    s_uint32_t        timeout_tick;                      /**< timeout tick */


}s_timer,*s_ptimer;

typedef struct thread
{
	void 			*psp;
	void 			*entry;
	void 			*stackaddr;
	s_uint32_t 	stacksize;
	s_list 		tlist;

	 /* priority */
    s_int8_t  current_priority;                       /**< current priority */
    s_uint8_t  init_priority;                          /**< initialized priority */
 	s_uint32_t  number_mask;

	s_uint32_t  init_tick;                                 /**< thread tick */
	s_uint32_t  remaining_tick;                          /**< thread remaining tick */
	
	s_timer    timer;                                  /**< thread timer */

}s_thread,*s_pthread;

typedef struct stack
{
	s_uint32_t r4;
	s_uint32_t r5;
	s_uint32_t r6;
	s_uint32_t r7;
	s_uint32_t r8;
	s_uint32_t r9;
	s_uint32_t r10;
	s_uint32_t r11;
	
	s_uint32_t r0;
	s_uint32_t r1;
	s_uint32_t r2;
	s_uint32_t r3;
	s_uint32_t r12;
	s_uint32_t lr;
	s_uint32_t pc;
	s_uint32_t psr;
}rtos_stack,*rtos_pstack;

#define s_inline static inline __attribute__((always_inline))



#define START_TIMER_GET_TIME 0x01
#define START_TIMER_SET_TIME 0x02

#endif

