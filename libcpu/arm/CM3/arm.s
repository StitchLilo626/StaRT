;******************************************************************************
; File: arm.s
; Brief: Cortex-M3 context switch & IRQ primitives (PendSV + PRIMASK helpers)
; Version: 1.0.2
; Date: 2025-08-26
; Author: StitchLilo626
; History:
;   2025-08-26 1.0.2 StitchLilo626: Translate Chinese comments to English (kept structure).
;------------------------------------------------------------------------------

    IMPORT  s_prev_thread_sp_p
    IMPORT  s_next_thread_sp_p
    IMPORT  s_interrupt_flag

SCB_VTOR        EQU     0xE000ED08
NVIC_INT_CTRL   EQU     0xE000ED04
NVIC_SYSPRI2    EQU     0xE000ED20
NVIC_PENDSV_PRI EQU     0x00FF0000
NVIC_PENDSVSET  EQU     0x10000000

    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

; s_uint32_t s_irq_disable(void);
s_irq_disable    PROC
    EXPORT  s_irq_disable
    MRS     r0, PRIMASK          ; save current PRIMASK
    CPSID   I                    ; disable IRQ
    BX      LR
    ENDP

; void s_irq_enable(s_uint32_t disirq);
s_irq_enable    PROC
    EXPORT  s_irq_enable
    MSR     PRIMASK, r0          ; restore PRIMASK
    BX      LR
    ENDP

; void s_first_switch_task(uint32_t to);
; First switch: no previous thread stack to save.
s_first_switch_task    PROC
    EXPORT s_first_switch_task

    LDR     r1, =s_next_thread_sp_p
    STR     r0, [r1]             ; store 'to' PSP pointer container

    LDR     r1, =s_prev_thread_sp_p
    MOV     r0, #0
    STR     r0, [r1]             ; mark no previous

    LDR     r1, =s_interrupt_flag
    MOV     r0, #1
    STR     r0, [r1]             ; set switch flag

    LDR     r0, =NVIC_SYSPRI2
    LDR     r1, =NVIC_PENDSV_PRI
    LDR.W   r2, [r0,#0x00]
    ORR     r1, r1, r2 
    STR     r1, [r0]             ; set PendSV lowest priority

    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]             ; trigger PendSV

    CPSIE   F
    CPSIE   I
    ENDP

; void s_normal_switch_task(uint32_t from, uint32_t to);
s_normal_switch_task    PROC
    EXPORT s_normal_switch_task

    LDR     r2, =s_interrupt_flag
    LDR     r3, [r2]
    CMP     r3, #1
    BEQ     _reswitch
    MOV     r3, #1
    STR     r3, [r2]             ; set flag first time

    LDR     r2, =s_prev_thread_sp_p
    STR     r0, [r2]             ; save prev PSP storage address

_reswitch
    LDR     r2, =s_next_thread_sp_p
    STR     r1, [r2]             ; store next PSP storage address

    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]             ; pend PendSV

    BX      LR
    ENDP

; PendSV_Handler: performs context save/restore using PSP
PendSV_Handler   PROC
    EXPORT PendSV_Handler

    MRS     r2, PRIMASK
    CPSID   I

    LDR     r0, =s_interrupt_flag
    LDR     r1, [r0]
    CBZ     r1, pendsv_exit
    MOV     r1, #0
    STR     r1, [r0]

    LDR     r0, =s_prev_thread_sp_p
    LDR     r1, [r0]
    CBZ     r1, switch_to_thread

    ; Save callee-saved registers r4-r11 onto old thread stack
    MRS     r1, psp
    STMFD   r1!, {r4 - r11}
    LDR     r0, [r0]
    STR     r1, [r0]

switch_to_thread
    LDR     r1, =s_next_thread_sp_p
    LDR     r1, [r1]
    LDR     r1, [r1]

    LDMFD   r1!, {r4 - r11}      ; restore callee-saved
    MSR     psp, r1

pendsv_exit
    MSR     PRIMASK, r2
    ORR     lr, lr, #0x04
    BX      lr
    ENDP

    ALIGN   4
    END

