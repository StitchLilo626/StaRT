/* SPDX-License-Identifier: MIT
 * GNU Cortex-M3 port. Frame layout matches cpuport.c and the Keil port.
 */
.syntax unified
.cpu cortex-m3
.thumb
.eabi_attribute 24, 1
.eabi_attribute 25, 1

.equ ICSR,          0xe000ed04
.equ SHPR3,         0xe000ed20
.equ PENDSVSET,     0x10000000
.equ PENDSV_LOWEST, 0x00ff0000

.section .text.s_irq_disable,"ax",%progbits
.global s_irq_disable
.type s_irq_disable,%function
.thumb_func
s_irq_disable:
    mrs r0, primask
    cpsid i
    bx lr
.size s_irq_disable, .-s_irq_disable

.section .text.s_irq_enable,"ax",%progbits
.global s_irq_enable
.type s_irq_enable,%function
.thumb_func
s_irq_enable:
    msr primask, r0
    bx lr
.size s_irq_enable, .-s_irq_enable

.section .text.s_first_switch_task,"ax",%progbits
.global s_first_switch_task
.type s_first_switch_task,%function
.thumb_func
s_first_switch_task:
    cpsid i
    ldr r1, =s_next_thread_sp_p
    str r0, [r1]
    ldr r1, =s_prev_thread_sp_p
    movs r0, #0
    str r0, [r1]
    ldr r1, =s_interrupt_flag
    movs r0, #1
    str r0, [r1]

    ldr r0, =SHPR3
    ldr r1, [r0]
    ldr r2, =PENDSV_LOWEST
    orrs r1, r1, r2
    str r1, [r0]
    ldr r0, =ICSR
    ldr r1, =PENDSVSET
    str r1, [r0]
    dsb
    cpsie f
    cpsie i
    isb
    /* First dispatch never returns or falls through into another function. */
.Lfirst_wait:
    b .Lfirst_wait
.size s_first_switch_task, .-s_first_switch_task
.ltorg

.section .text.s_normal_switch_task,"ax",%progbits
.global s_normal_switch_task
.type s_normal_switch_task,%function
.thumb_func
s_normal_switch_task:
    /* Publish the complete request atomically, including calls from an ISR. */
    mrs r12, primask
    cpsid i
    ldr r2, =s_interrupt_flag
    ldr r3, [r2]
    cbnz r3, .Lreplace_next
    movs r3, #1
    str r3, [r2]
    ldr r2, =s_prev_thread_sp_p
    str r0, [r2]
.Lreplace_next:
    /* A second pending request preserves the original outgoing PSP. */
    ldr r2, =s_next_thread_sp_p
    str r1, [r2]
    ldr r0, =ICSR
    ldr r1, =PENDSVSET
    str r1, [r0]
    dsb
    msr primask, r12
    isb
    bx lr
.size s_normal_switch_task, .-s_normal_switch_task
.ltorg

.section .text.PendSV_Handler,"ax",%progbits
.global PendSV_Handler
.type PendSV_Handler,%function
.thumb_func
PendSV_Handler:
    mrs r2, primask
    cpsid i
    ldr r0, =s_interrupt_flag
    ldr r1, [r0]
    cbz r1, .Lpendsv_return
    movs r1, #0
    str r1, [r0]

    ldr r0, =s_prev_thread_sp_p
    ldr r1, [r0]
    cbz r1, .Lrestore_context
    mrs r1, psp
    stmdb r1!, {r4-r11}
    ldr r0, [r0]
    str r1, [r0]
.Lrestore_context:
    ldr r1, =s_next_thread_sp_p
    ldr r1, [r1]
    ldr r1, [r1]
    ldmia r1!, {r4-r11}
    msr psp, r1
    isb
    /* Basic exception frame, privileged Thread mode using PSP (no FPU). */
    ldr lr, =0xfffffffd
.Lpendsv_return:
    msr primask, r2
    bx lr
.size PendSV_Handler, .-PendSV_Handler
.ltorg

.section .note.GNU-stack,"",%progbits
