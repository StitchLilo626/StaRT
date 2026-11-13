/* SPDX-License-Identifier: MIT */
#include "cpuport.h"
#include "start.h"
#include <stdint.h>

#if !defined(__ARM_ARCH_7EM__) || !defined(__ARM_FP) || !(__ARM_FP & 4)
#error "The Cortex-M4F port requires FPv4-SP support"
#endif

void s_cm4f_fpu_init(void)
{
    volatile uint32_t *const cpacr = (volatile uint32_t *)0xe000ed88UL;
    volatile uint32_t *const fpccr = (volatile uint32_t *)0xe000ef34UL;
    *cpacr |= 0x00f00000UL; /* Full access to CP10 and CP11. */
    __asm volatile("dsb\nisb" ::: "memory");
    /* Track FP-active threads and stack their low FP registers on exception entry. */
    *fpccr = (*fpccr | (1UL << 31)) & ~(1UL << 30); /* ASPEN=1, LSPEN=0 */
    __asm volatile("dsb\nisb" ::: "memory");
}

s_uint8_t *s_stack_init(s_thread_entry_t entry, s_uint8_t *stackaddr)
{
    /* r4-r11, EXC_RETURN, then the eight words of a basic exception frame. */
    uintptr_t top = (uintptr_t)stackaddr & ~(uintptr_t)7;
    uint32_t *frame = (uint32_t *)top - 17;
    for (unsigned int word = 0; word < 17; ++word)
        frame[word] = 0;
    frame[8] = 0xfffffffdUL; /* Thread mode, PSP, basic frame. */
    frame[14] = (uintptr_t)s_thread_exit;
    frame[15] = (uintptr_t)entry & ~(uintptr_t)1;
    frame[16] = 0x01000000UL; /* xPSR.T */
    return (s_uint8_t *)frame;
}

int __s_ffs(s_uint32_t value)
{
    return __builtin_ffs(value);
}
