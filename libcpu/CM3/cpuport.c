/**
 * @file cpuport.c
 * @brief Cortex-M3 CPU port: stack frame initialization & ffs helper.
 * @version 1.0.2
 * @date 2025-08-26
 * author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 Translate comments to English.
 */

#include "start.h"

/**
 * @brief Saved register frame (software stacked + hardware stacked).
 * Layout matches push/pop sequence for context switch.
 */
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

    /* Hardware-stacked on exception entry */
    s_uint32_t r0;
    s_uint32_t r1;
    s_uint32_t r2;
    s_uint32_t r3;
    s_uint32_t r12;
    s_uint32_t lr;
    s_uint32_t pc;
    s_uint32_t psr;
} s_stack, *s_pstack;

/**
 * @brief Initialize thread stack frame for first run.
 * @param entry Thread entry function.
 * @param stackaddr Stack top (high address end).
 * @return New PSP value after frame placement.
 */
s_uint8_t *s_stack_init(void *entry, s_uint8_t *stackaddr)
{
    s_pstack   pstack;
    s_uint8_t *psp;
    s_uint8_t  i;

    psp = stackaddr;

    /* 8-byte align per ARM procedure call & exception entry requirements. */
    psp = (s_uint8_t *)(((s_uint32_t)psp) & ~((8) - 1));

    /* Reserve space for initial context frame */
    psp -= sizeof(s_stack);
    pstack = (s_pstack)psp;

    /* Clear frame area */
    for (i = 0; i < 16; i++)
        ((s_uint32_t *)pstack)[i] = 0;

    pstack->psr = 0x01000000UL;         /* Default xPSR (Thumb bit set) */
    pstack->pc  = (s_uint32_t)entry;    /* Entry point */
    pstack->lr  = (s_uint32_t)s_thread_exit; /* If thread function returns */

    return psp;
}

#if START_USING_CPU_FFS
/* Architecture-specific __s_ffs provided in assembly/inline blocks below. */
#if defined(__CC_ARM)
__asm int __s_ffs(int value)
{
    CMP     r0, #0x00
    BEQ     exit
    RBIT    r0, r0
    CLZ     r0, r0
    ADDS    r0, r0, #0x01
exit
    BX      lr
}
#elif defined(__CLANG_ARM)
int __s_ffs(int value)
{
    __asm volatile(
        "CMP     r0, #0x00            \n"
        "BEQ     1f                   \n"
        "RBIT    r0, r0               \n"
        "CLZ     r0, r0               \n"
        "ADDS    r0, r0, #0x01        \n"
        "1:                           \n"
        : "=r"(value)
        : "r"(value)
    );
    return value;
}
#elif defined(__IAR_SYSTEMS_ICC__)
int __s_ffs(int value)
{
    if (value == 0) return value;
    asm("RBIT %0, %1" : "=r"(value) : "r"(value));
    asm("CLZ  %0, %1" : "=r"(value) : "r"(value));
    asm("ADDS %0, %1, #0x01" : "=r"(value) : "r"(value));
    return value;
}
#elif defined(__GNUC__)
int __s_ffs(int value)
{
    return __builtin_ffs(value);
}
#endif
#else
/**
 * @brief Generic fallback implementation of __s_ffs (first set bit).
 */
int __s_ffs(int value)
{
#if defined(__GNUC__) || defined(__CLANG_ARM)
    return __builtin_ffs(value);
#else
    if (value == 0) return 0;
    int idx = 1;
    while ((value & 1) == 0) { value >>= 1; ++idx; }
    return idx;
#endif
}
#endif
