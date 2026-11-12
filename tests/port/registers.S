.syntax unified
.cpu cortex-m3
.thumb
.section .text.smoke_context_probe,"ax",%progbits
.global smoke_context_probe
.type smoke_context_probe,%function
.thumb_func
smoke_context_probe:
    push {r3-r11, lr} /* Ten words preserve AAPCS 8-byte call alignment. */
    mov r4, #0x44
    mov r5, #0x55
    mov r6, #0x66
    mov r7, #0x77
    mov r8, #0x88
    mov r9, #0x99
    mov r10, #0xaa
    mov r11, #0xbb
    bl smoke_request_switch
    cmp r4, #0x44
    bne .Lfail
    cmp r5, #0x55
    bne .Lfail
    cmp r6, #0x66
    bne .Lfail
    cmp r7, #0x77
    bne .Lfail
    cmp r8, #0x88
    bne .Lfail
    cmp r9, #0x99
    bne .Lfail
    cmp r10, #0xaa
    bne .Lfail
    cmp r11, #0xbb
    bne .Lfail
    movs r0, #1
    pop {r3-r11, pc}
.Lfail:
    movs r0, #0
    pop {r3-r11, pc}
.size smoke_context_probe, .-smoke_context_probe
.section .note.GNU-stack,"",%progbits
