.module start_mcs51_context

.globl _s_irq_disable, _s_irq_enable
.globl _s_mcs51_switch_isr, _s_mcs51_stack_fault
.globl _s_mcs51_outgoing, _s_mcs51_incoming, _s_mcs51_pending
.globl __start__stack, l_SSEG, _bp, s_BIT_BANK

SP  = 0x81
DPL = 0x82
DPH = 0x83
PSW = 0xd0
ACC = 0xe0
B   = 0xf0
EA  = 0xaf
TF0 = 0x8d

.area DSEG (DATA)
saved_acc: .ds 1
saved_psw: .ds 1

; CMake can pass the assembly object before main. Keep the SDCC vector and
; startup areas first even when this object establishes the link order.
.area HOME    (CODE)
.area GSINIT0 (CODE)
.area GSINIT1 (CODE)
.area GSINIT2 (CODE)
.area GSINIT3 (CODE)
.area GSINIT4 (CODE)
.area GSINIT5 (CODE)
.area GSINIT  (CODE)
.area GSFINAL (CODE)
.area CSEG (CODE)

; SDCC returns an unsigned long in DPL, DPH, B, ACC.
_s_irq_disable:
    mov  dpl,#0
    mov  dph,#0
    mov  b,#0
    clr  a
    jbc  EA,irq_was_enabled
    ret
irq_was_enabled:
    inc  dpl
    ret

_s_irq_enable:
    mov  a,dpl
    jz   irq_stays_disabled
    setb EA
    ret
irq_stays_disabled:
    clr  EA
    ret

; Timer 0 is stopped and its flag is software-pended. Timer 1 runs the tick at
; a higher priority so multiple requests can arrive before this ISR starts.
_s_mcs51_switch_isr:
    clr  EA
    clr  TF0
    mov  saved_acc,a
    mov  saved_psw,PSW
    mov  a,_s_mcs51_pending
    jz   no_switch
    mov  _s_mcs51_pending,#0
    mov  a,_s_mcs51_outgoing
    cjne a,_s_mcs51_incoming,save_context
    mov  a,(_s_mcs51_outgoing + 1)
    cjne a,(_s_mcs51_incoming + 1),save_context
no_switch:
    mov  PSW,saved_psw
    mov  a,saved_acc
    setb EA
    reti

save_context:
    ; Leave room for the 15 software-saved bytes before pushing them.
    mov  a,sp
    add  a,#15
    jc   stack_fault_near
    clr  c
    subb a,#(__start__stack - 1)
    jc   stack_fault_near
    cjne a,#l_SSEG,check_save_limit
    sjmp push_context
check_save_limit:
    jc   push_context
stack_fault_near:
    ljmp _s_mcs51_stack_fault
push_context:
    mov  a,saved_acc
    push ACC
    push B
    push DPL
    push DPH
    push saved_psw
    push 0x00
    push 0x01
    push 0x02
    push 0x03
    push 0x04
    push 0x05
    push 0x06
    push 0x07
    push _bp
    push s_BIT_BANK

    mov  dpl,_s_mcs51_outgoing
    mov  dph,(_s_mcs51_outgoing + 1)
    mov  a,dpl
    orl  a,dph
    jz   restore_context
    mov  r0,#__start__stack
    mov  a,sp
    clr  c
    subb a,r0
    jc   _s_mcs51_stack_fault
    inc  a
    mov  r1,a
    movx a,@dptr
    clr  c
    subb a,r1
    jc   _s_mcs51_stack_fault
    inc  dptr
    mov  a,r1
    movx @dptr,a
save_stack:
    inc  dptr
    mov  a,@r0
    movx @dptr,a
    inc  r0
    djnz r1,save_stack

restore_context:
    mov  dpl,_s_mcs51_incoming
    mov  dph,(_s_mcs51_incoming + 1)
    movx a,@dptr
    mov  r2,a
    inc  dptr
    movx a,@dptr
    mov  r1,a
    clr  c
    subb a,#17
    jc   _s_mcs51_stack_fault
    mov  a,r2
    clr  c
    subb a,r1
    jc   _s_mcs51_stack_fault
    mov  a,r1
    cjne a,#l_SSEG,check_restore_limit
    sjmp set_stack_pointer
check_restore_limit:
    jnc  _s_mcs51_stack_fault
set_stack_pointer:
    add  a,#(__start__stack - 1)
    jc   _s_mcs51_stack_fault
    mov  sp,a
    mov  r0,#__start__stack
restore_stack:
    inc  dptr
    movx a,@dptr
    mov  @r0,a
    inc  r0
    djnz r1,restore_stack

    pop  s_BIT_BANK
    pop  _bp
    pop  0x07
    pop  0x06
    pop  0x05
    pop  0x04
    pop  0x03
    pop  0x02
    pop  0x01
    pop  0x00
    pop  PSW
    pop  DPH
    pop  DPL
    pop  B
    pop  ACC
    setb EA
    reti

; A C call here could overwrite an already exhausted stack.
_s_mcs51_stack_fault:
    nop
    sjmp _s_mcs51_stack_fault
