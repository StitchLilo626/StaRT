/* SPDX-License-Identifier: MIT
 * GNU Cortex-M3 port.
 * Frame layout matches cpuport.c and the Keil port.
 */
.syntax unified
.cpu cortex-m3
.thumb
.eabi_attribute 24, 1
.eabi_attribute 25, 1

#include "../common/switch_request_gcc.inc"

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
