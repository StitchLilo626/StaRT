/* SPDX-License-Identifier: MIT */
.section .text.Reset_Handler,"ax",%progbits
.global Reset_Handler
.type Reset_Handler,%function
.thumb_func
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
.Lcopy_data:
    cmp r1, r2
    bcs .Lzero_start
    ldr r3, [r0], #4
    str r3, [r1], #4
    b .Lcopy_data
.Lzero_start:
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
.Lzero_bss:
    cmp r1, r2
    bcs .Linitialize
    str r3, [r1], #4
    b .Lzero_bss
.Linitialize:
    /* Explicit VTOR also supports debugger entry without boot alias remapping. */
    ldr r0, =0xe000ed08
    ldr r1, =g_pfnVectors
    str r1, [r0]
    dsb
    isb
    bl SystemInit
    bl __libc_init_array
    bl main
.Lmain_returned:
    cpsid i
    b .Lmain_returned
.size Reset_Handler, .-Reset_Handler
.ltorg

/* newlib's constructor dispatcher calls _init; there is no crt0/OS runtime. */
.section .init,"ax",%progbits
.global _init
.type _init,%function
.thumb_func
_init:
    bx lr
.size _init, .-_init

.section .text.Default_Handler,"ax",%progbits
.global Default_Handler
.type Default_Handler,%function
.thumb_func
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler

.macro default_handler name
    .weak \name
    .thumb_set \name, Default_Handler
.endm
