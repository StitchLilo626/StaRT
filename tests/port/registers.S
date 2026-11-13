.syntax unified
.thumb
.section .text.smoke_context_probe,"ax",%progbits
.global smoke_context_probe
.type smoke_context_probe,%function
.thumb_func
smoke_context_probe:
    push {r3-r11, lr} /* Ten words preserve AAPCS 8-byte call alignment. */
#if defined(__ARM_PCS_VFP)
    vpush {s16-s31}
    ldr r0, =.Lfp_pattern
    vldmia r0, {s0-s31}
    mov r0, #0x00400000
    vmsr fpscr, r0
#endif
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
#if defined(__ARM_PCS_VFP)
    vmrs r0, fpscr
    cmp r0, #0x00400000
    bne .Lfail
    sub sp, sp, #128
    mov r0, sp
    vstmia r0, {s0-s31}
    ldr r1, =.Lfp_pattern
    movs r2, #32
.Lcheck_fp:
    ldr r3, [r0], #4
    ldr r4, [r1], #4
    cmp r3, r4
    bne .Lfp_fail
    subs r2, #1
    bne .Lcheck_fp
    add sp, sp, #128
#endif
    movs r0, #1
    b .Ldone
#if defined(__ARM_PCS_VFP)
.Lfp_fail:
    add sp, sp, #128
#endif
.Lfail:
    movs r0, #0
.Ldone:
#if defined(__ARM_PCS_VFP)
    movs r1, #0
    vmsr fpscr, r1
    vpop {s16-s31}
#endif
    pop {r3-r11, pc}
.size smoke_context_probe, .-smoke_context_probe
#if defined(__ARM_PCS_VFP)

.global smoke_fp_switch_back
.type smoke_fp_switch_back,%function
.thumb_func
smoke_fp_switch_back:
    /* Deliberately overwrite every register before requesting the switch. */
    push {r4, lr}
    vpush {s16-s31}
    ldr r2, =.Lfp_other
    vldmia r2, {s0-s31}
    mov r2, #0x00800000
    vmsr fpscr, r2
    bl s_normal_switch_task
    vmrs r0, fpscr
    cmp r0, #0x00800000
    bne .Lother_fail
    sub sp, sp, #128
    mov r0, sp
    vstmia r0, {s0-s31}
    movs r1, #32
    movs r2, #0
.Lcheck_other:
    ldr r3, [r0], #4
    orrs r2, r3
    subs r1, #1
    bne .Lcheck_other
    add sp, sp, #128
    cbnz r2, .Lother_fail
    movs r0, #1
    b .Lother_done
.Lother_fail:
    movs r0, #0
.Lother_done:
    movs r1, #0
    vmsr fpscr, r1
    vpop {s16-s31}
    pop {r4, pc}
.size smoke_fp_switch_back, .-smoke_fp_switch_back
.ltorg

.section .rodata.smoke_fp,"a",%progbits
.balign 4
.Lfp_pattern:
.set fp_word, 0x3f800000
.rept 32
    .word fp_word
    .set fp_word, fp_word + 0x10000
.endr
.Lfp_other:
    .space 128
#endif
.section .note.GNU-stack,"",%progbits
