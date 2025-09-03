/**
 * @file sdef.h
 * @brief Kernel fundamental type and structure definitions.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate Chinese member comments to English.
 */

#ifndef __TYPEDEF_H_
#define __TYPEDEF_H_

#include "StaRT_Config.h"

/* Fixed width integer aliases */
typedef signed char         s_int8_t;
typedef unsigned char       s_uint8_t;
typedef short               s_int16_t;
typedef unsigned short      s_uint16_t;
typedef int                 s_int32_t;
typedef unsigned int        s_uint32_t;
typedef long long           s_int64_t;
typedef unsigned long long  s_uint64_t;

/**
 * @brief Generic status codes used across subsystems.
 */
typedef enum
{
    S_OK = 0,          /**< Operation succeeded */
    S_ERR = -1,        /**< Generic error */
    S_TIMEOUT = -2,    /**< Timeout expired */
    S_BUSY = -3,       /**< Resource busy */
    S_INVALID = -4,    /**< Invalid argument */
    S_NULL = -5,       /**< NULL pointer */
    S_DELETED = -6,    /**< Object deleted / invalid */
    S_UNSUPPORTED = -7 /**< Unsupported operation */
} s_status;

/**
 * @brief Intrusive doubly-linked list node.
 */
typedef struct list
{
    struct list *next;
    struct list *prev;
} s_list, *s_plist;

/**
 * @brief Software timer control block.
 */
typedef struct timer
{
    s_list      row[START_TIMER_SKIP_LIST_LEVEL]; /**< Skip-list / level nodes */
    void      (*timeout_func)(void *p);           /**< Timeout callback */
    void       *p;                                /**< User parameter */
    s_uint32_t  init_tick;                        /**< Initial duration (ticks) */
    s_uint32_t  timeout_tick;                     /**< Absolute expiration tick */
} s_timer, *s_ptimer;

/**
 * @brief Thread control block.
 */
typedef struct thread
{
    void       *psp;              /**< Saved process stack pointer (hardware context next restore point) */
    void       *entry;            /**< Entry function */
    void       *stackaddr;        /**< Stack base (low address) */
    s_uint32_t  stacksize;        /**< Stack size in bytes */
    s_list      tlist;            /**< Run / wait queue list node */

    s_uint8_t   current_priority; /**< Current (possibly boosted) priority */
    s_uint8_t   init_priority;    /**< Original priority at creation */
    s_uint32_t  number_mask;      /**< Bit mask for ready group */

    s_uint32_t  init_tick;        /**< Time slice length (ticks) */
    s_uint32_t  remaining_tick;   /**< Remaining time slice */
    s_int32_t   status;           /**< Thread lifecycle status flags */
    s_timer     timer;            /**< Per-thread sleep/timeout timer */
} s_thread, *s_pthread;
#if START_USING_IPC
/**
 * @brief Common IPC parent header embedded in IPC objects.
 */
struct ipc_parent
{
    s_uint8_t status;             /**< 1 = valid, 0 = deleted */
    s_uint8_t flag;               /**< Queueing policy / mode */
    s_list    suspend_thread;     /**< Waiting thread list head */
};

#if START_USING_SEMAPHORE
/**
 * @brief Counting semaphore object.
 */
typedef struct semaphore
{
    struct ipc_parent parent; /**< Base IPC header */
    s_uint16_t count;         /**< Current resource count */
    s_uint16_t reserved;      /**< Reserved (alignment/extension) */
} s_sem, *s_psem;
#endif

#if START_USING_MUTEX
/**
 * @brief Mutex with recursion + simple priority inheritance.
 */
typedef struct mutex
{
    struct ipc_parent parent;       /**< Base IPC header */
    s_pthread          owner;       /**< Owning thread */
    s_uint16_t         count;       /**< Availability (1 free, 0 taken) */
    s_uint8_t          original_priority; /**< Owner original priority before inheritance */
    s_uint8_t          hold;        /**< Recursive acquisition depth */
} s_mutex, *s_pmutex;
#endif

#if START_USING_MESSAGEQUEUE
/**
 * @brief Message queue control block.
 */
typedef struct
{
    struct ipc_parent parent;          /**< Base IPC header */
    void             *msg_pool;        /**< Raw pool base */
    s_uint16_t        msg_size;        /**< Aligned payload size */
    s_uint16_t        max_msgs;        /**< Maximum storable messages */
    s_uint16_t        index;           /**< Current queued count */
    void             *msg_queue_head;  /**< FIFO head (linked nodes) */
    void             *msg_queue_tail;  /**< FIFO tail */
    void             *msg_queue_free;  /**< Free node stack head */
    s_list            suspend_sender_thread; /**< Sender wait list */
} s_msgqueue, *s_pmsgqueue;

/**
 * @brief Internal message node header preceding payload.
 */
struct s_mq_message
{
    struct s_mq_message *next; /**< Singly linked list next pointer */
    /* Payload bytes follow immediately */
};
#endif

#endif

#define s_inline static inline __attribute__((always_inline))

/* Thread status flags */
#define START_THREAD_READY       0x01
#define START_THREAD_SUSPEND     0x02
#define START_THREAD_TERMINATED  0x08
#define START_THREAD_RUNNING     0x10
#define START_THREAD_DELETED     0x20
#define START_THREAD_INIT        0x80

/* Timer control command codes */
#define START_TIMER_GET_TIME     0x01
#define START_TIMER_SET_TIME     0x02

/* Thread control commands */
#define START_THREAD_GET_STATUS    0x01
#define START_THREAD_SET_STATUS    0x02
#define START_THREAD_GET_PRIORITY  0x03
#define START_THREAD_SET_PRIORITY  0x04

#if START_DEBUG
#define START_DEBUG_INFO 0x01
#define START_DEBUG_WARN 0x02
#define START_DEBUG_ERR  0x03
#endif

#if START_USING_IPC
#define START_IPC_FLAG_FIFO  0x00 /**< FIFO ordering */
#define START_IPC_FLAG_PRIO  0x01 /**< Priority ordering (lower numeric = higher priority) */
#define START_WAITING_FOREVER ((s_int32_t)(-1)) /**< Block forever */
#define START_WAITING_NO      ((s_int32_t)(0))  /**< Non-blocking */

#if START_USING_SEMAPHORE
#define SEM_VALUE_MAX 0xFFFF
#endif

#if START_USING_MUTEX
#define MUTEX_HOLD_MAX 0xFF
#endif

#if START_USING_MESSAGEQUEUE
/**
 * @brief Compute memory pool size for a message queue.
 * @param msg_size Raw single message size.
 * @param msg_count Message count.
 */
#define START_MSGQ_POOL_SIZE(msg_size, msg_count) \
    ( (size_t)(msg_count) * ( START_ALIGN_UP((size_t)(msg_size), START_ALIGN_SIZE) + sizeof(struct s_mq_message) ) )
#endif

#endif



#ifndef __weak
#define __weak  __attribute__((weak))
#endif

#if START_DEBUG
/**
 * @brief Simple formatted debug logging macro (disabled when START_DEBUG=0).
 */
#define S_DEBUG_LOG(level, fmt, ...)                                              \
    do {                                                                          \
        const char *slevel =                                                      \
            ((level) == START_DEBUG_ERR)  ? "ERR" :                               \
            ((level) == START_DEBUG_WARN) ? "WARN" : "INFO";                      \
        s_printf("[%s] " fmt, slevel, ##__VA_ARGS__);                             \
    } while (0)
#else
#define S_DEBUG_LOG(level, fmt, ...) ((void)0)
#endif

#define START_ALIGN_SIZE 4
#define START_ALIGN_UP(sz, a) ( ((sz) + ((a)-1)) & ~((a)-1) )


/**
 * @brief Obtain container struct pointer from member pointer.
 */
#define S_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @brief Get struct pointer that embeds a list node.
 */
#define S_LIST_ENTRY(node, type, member) \
    S_CONTAINER_OF(node, type, member)

#endif /* __TYPEDEF_H_ */

