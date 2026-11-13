/* SPDX-License-Identifier: MIT */
.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
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
    cbz r1, .Lreturn
    movs r1, #0
    str r1, [r0]

    ldr r0, =s_prev_thread_sp_p
    ldr r0, [r0]
    cbz r0, .Lrestore
    mrs r1, psp
    /* With an extended frame the hardware saves s0-s15 and FPSCR. */
    tst lr, #0x10
    it eq
    vstmdbeq r1!, {s16-s31}
    stmdb r1!, {r4-r11, lr}
    str r1, [r0]

.Lrestore:
    ldr r1, =s_next_thread_sp_p
    ldr r1, [r1]
    ldr r1, [r1]
    ldmia r1!, {r4-r11, lr}
    tst lr, #0x10
    it eq
    vldmiaeq r1!, {s16-s31}
    msr psp, r1
    isb
.Lreturn:
    msr primask, r2
    bx lr
.size PendSV_Handler, .-PendSV_Handler
.ltorg

.section .note.GNU-stack,"",%progbits
