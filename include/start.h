/**
 * @file start.h
 * @brief Core public kernel API declarations.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @copyright
 *   (c) 2025 StaRT Project
 * @note
 *   History:
 *     - 2025-08-26 1.0.1 GitHub Copilot: Added Doxygen comments and normalized English API docs.
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate in-function Chinese comments to English, author updated.
 */

#ifndef __START_H_
#define __START_H_

#include "sdef.h"
#include <stddef.h>

/** Pointer to currently running thread (NULL before scheduler start). */
extern s_pthread s_current_thread;
/** Priority of currently running thread. */
extern s_uint8_t s_current_priority;

/**
 * @brief Perform a normal context switch (assembly implementation).
 * @param prev Address of previous thread PSP storage location.
 * @param next Address of next thread PSP storage location.
 */
void s_normal_switch_task(s_uint32_t prev, s_uint32_t next);

/**
 * @brief First context switch to start scheduling (assembly implementation).
 * @param next Address of next thread PSP storage location.
 * @note Spelling kept for backward compatibility (frist->first).
 */
void s_first_switch_task(s_uint32_t next);

/**
 * @brief Disable interrupts.
 * @return Previous PRIMASK state (pass to s_irq_enable()).
 */
s_uint32_t s_irq_disable(void);

/**
 * @brief Restore interrupts to previous state.
 * @param disirq Saved PRIMASK returned by s_irq_disable().
 */
void s_irq_enable(s_uint32_t disirq);

/**
 * @brief Initialize a thread stack frame (Cortex-M PSP layout).
 * @param entry Thread entry function.
 * @param stackaddr Top address of stack (end of buffer).
 * @return PSP pointer after context frame prepared.
 */
s_uint8_t *s_stack_init(void *entry, s_uint8_t *stackaddr);

/* Doubly linked intrusive list primitives */
void s_list_init(s_plist l);
void s_list_insert_after(s_plist l, s_plist n);
void s_list_insert_before(s_plist l, s_plist n);
void s_list_delete(s_plist d);
int  s_list_isempty(s_plist l);

/**
 * @brief Get current running thread.
 * @return Thread object or NULL before scheduler start.
 */
s_pthread s_thread_get(void);

/**
 * @brief Initialize core subsystems (scheduler, timer, idle thread, banner).
 * @return S_OK on success.
 */
s_status s_start_init(void);

/**
 * @brief Weak hook to print startup banner (can be overridden).
 */
void s_start_banner(void);

/**
 * @brief Formatted output (minimal printf subset).
 * @param fmt Format string.
 */
void s_printf(const char *fmt, ...);

/* Scheduler control APIs */
void s_sched_init(void);
void s_sched_start(void);
void s_sched_switch(void);
void s_sched_remove_thread(s_pthread thread);
void s_sched_insert_thread(s_pthread thread);
void s_thread_yield(void);
void s_cleanup_defunct_threads(void);

/**
 * @brief Put current thread to sleep (block) for tick count.
 * @param tick Number of ticks to sleep.
 */
void s_thread_sleep(s_uint32_t tick);

/**
 * @brief Terminate current thread (deferred cleanup by idle).
 */
void s_thread_exit(void);

/**
 * @brief Initialize a thread object (not yet ready).
 * @param thread Thread control block.
 * @param entry Entry function pointer.
 * @param stackaddr Stack memory base address.
 * @param stacksize Stack size in bytes.
 * @param priority Thread priority (0 = highest).
 * @param tick Time slice (ticks).
 * @return S_OK on success, else error code.
 */
s_status s_thread_init(s_pthread thread,void *entry,void *stackaddr,s_uint32_t stacksize,s_int8_t priority,s_uint32_t tick);

/**
 * @brief Move an initialized thread into ready state.
 * @param thread Thread object.
 * @return S_OK on success.
 */
s_status s_thread_startup(s_pthread thread);

/* Thread lifecycle / control */
s_status s_thread_delete(s_pthread thread);
s_status s_thread_suspend(s_pthread thread);
s_status s_thread_ctrl(s_pthread thread, s_uint32_t cmd, void *arg);
s_status s_thread_restart(s_pthread thread);

/* Timer subsystem */
void      s_timer_list_init(void);
void      s_mdelay(s_uint32_t ms);
void      s_delay(s_uint32_t tick);
s_status  s_timer_init(s_ptimer timer, void (*timeout_func)(void *p), void *p, s_uint32_t tick);
void      s_timer_check(void);
void      s_tick_increase(void);
s_uint32_t s_tick_get(void);
s_status  s_timer_ctrl(s_ptimer timer, s_uint32_t cmd, void *arg);
s_status  s_timer_stop(s_ptimer timer);
s_status  s_timer_start(s_ptimer timer);
void      timeout_function(void *p);

#if START_USING_IPC
/* IPC: semaphore / mutex / message queue APIs */
#if START_USING_SEMAPHORE
s_status s_sem_init(s_psem sem, s_uint16_t value, s_uint8_t flag);
s_status s_sem_delete(s_psem sem);
s_status s_sem_take(s_psem sem, s_int32_t time);
s_status s_sem_release(s_psem sem);
#endif
#if START_USING_MUTEX
s_status s_mutex_init(s_pmutex m, s_uint8_t flag);
s_status s_mutex_delete(s_pmutex m);
s_status s_mutex_take(s_pmutex m, s_int32_t time);
s_status s_mutex_release(s_pmutex m);
#endif
#if START_USING_MESSAGEQUEUE
s_status s_msgqueue_init(s_pmsgqueue mq, void *msg_pool, s_uint16_t msg_size, s_uint16_t pool_size, s_uint8_t flag);
s_status s_msgqueue_delete(s_pmsgqueue mq);
s_status s_msgqueue_send_wait(s_pmsgqueue mq, const void *buffer, s_uint16_t size, s_int32_t timeout);
s_status s_msgqueue_send(s_pmsgqueue mq, const void *buffer, s_uint16_t size);
s_status s_msgqueue_urgent(s_pmsgqueue mq, const void *buffer, s_uint16_t size);
s_status s_msgqueue_recv(s_pmsgqueue mq, void *buffer, s_uint16_t size, s_int32_t timeout);
#endif

#endif
/**
 * @brief Find first (least significant) bit set.
 * @param value Input value.
 * @return Bit index starting at 1, or 0 if value is 0.
 */
int __s_ffs(int value);

#endif


