#include <start.h>


s_uint8_t *rtos_stack_init(void *entry, s_uint8_t *stackaddr)
{
    rtos_pstack pstack;
    s_uint8_t *psp;
    s_uint8_t i;
    psp = stackaddr;
    // 8字节对齐，保证栈指针满足Cortex-M3要求
    psp = (s_uint8_t *)(((s_uint32_t)psp) & ~((8) - 1));
    // 预留一块空间用于保存初始寄存器快照
    psp -= sizeof(rtos_stack);
    pstack = (rtos_pstack)psp;
    // 清零初始寄存器快照
    for(i=0;i<16;i++)
    {
        ((s_uint32_t *)pstack)[i] = 0;
    }
    // 设置xPSR寄存器，Thumb状态位必须为1
    pstack->psr = 0x01000000L;
    // 设置PC为线程入口函数地址
    pstack->pc = (s_uint32_t)entry;
    // 返回初始化后的栈指针
    return psp;
}

void system_init()
{
    // 系统初始化代码
    // 这里可以添加系统时钟配置、外设初始化等代码
}

void systick_init()
{
    // 系统滴答定时器初始化代码
    // 这里可以添加系统滴答定时器的配置和启动代码
}
void systick_handler()
{
    // 系统滴答定时器中断处理函数
    // 这里可以添加定时器中断处理逻辑，例如更新系统时间、调度线程等
   s_tick_increase(); 
}

#ifdef START_USING_CPU_FFS
/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
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

#endif
