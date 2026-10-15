/* SPDX-License-Identifier: MIT
 * StaRT GNU startup for the STM32F103 medium-density interrupt map.
 * Keil's startup and the ST vendor sources remain independent and unchanged.
 */
.syntax unified
.cpu cortex-m3
.thumb
.eabi_attribute 24, 1
.eabi_attribute 25, 1

.section .isr_vector,"a",%progbits
.balign 512
.global g_pfnVectors, __Vectors
.type g_pfnVectors,%object
g_pfnVectors:
__Vectors:
    .word _estack, Reset_Handler
    .word NMI_Handler, HardFault_Handler, MemManage_Handler
    .word BusFault_Handler, UsageFault_Handler
    .word 0, 0, 0, 0
    .word SVC_Handler, DebugMon_Handler, 0, PendSV_Handler, SysTick_Handler
    .word WWDG_IRQHandler, PVD_IRQHandler, TAMPER_IRQHandler, RTC_IRQHandler
    .word FLASH_IRQHandler, RCC_IRQHandler
    .word EXTI0_IRQHandler, EXTI1_IRQHandler, EXTI2_IRQHandler
    .word EXTI3_IRQHandler, EXTI4_IRQHandler
    .word DMA1_Channel1_IRQHandler, DMA1_Channel2_IRQHandler
    .word DMA1_Channel3_IRQHandler, DMA1_Channel4_IRQHandler
    .word DMA1_Channel5_IRQHandler, DMA1_Channel6_IRQHandler
    .word DMA1_Channel7_IRQHandler, ADC1_2_IRQHandler
    .word USB_HP_CAN1_TX_IRQHandler, USB_LP_CAN1_RX0_IRQHandler
    .word CAN1_RX1_IRQHandler, CAN1_SCE_IRQHandler, EXTI9_5_IRQHandler
    .word TIM1_BRK_IRQHandler, TIM1_UP_IRQHandler, TIM1_TRG_COM_IRQHandler
    .word TIM1_CC_IRQHandler, TIM2_IRQHandler, TIM3_IRQHandler, TIM4_IRQHandler
    .word I2C1_EV_IRQHandler, I2C1_ER_IRQHandler
    .word I2C2_EV_IRQHandler, I2C2_ER_IRQHandler
    .word SPI1_IRQHandler, SPI2_IRQHandler
    .word USART1_IRQHandler, USART2_IRQHandler, USART3_IRQHandler
    .word EXTI15_10_IRQHandler, RTC_Alarm_IRQHandler, USBWakeUp_IRQHandler
.size g_pfnVectors, .-g_pfnVectors

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

/* PendSV and SysTick deliberately have no weak fallback: missing RTOS/board
 * handlers must fail the firmware link instead of silently producing an image.
 */
default_handler NMI_Handler
default_handler HardFault_Handler
default_handler MemManage_Handler
default_handler BusFault_Handler
default_handler UsageFault_Handler
default_handler SVC_Handler
default_handler DebugMon_Handler
default_handler WWDG_IRQHandler
default_handler PVD_IRQHandler
default_handler TAMPER_IRQHandler
default_handler RTC_IRQHandler
default_handler FLASH_IRQHandler
default_handler RCC_IRQHandler
default_handler EXTI0_IRQHandler
default_handler EXTI1_IRQHandler
default_handler EXTI2_IRQHandler
default_handler EXTI3_IRQHandler
default_handler EXTI4_IRQHandler
default_handler DMA1_Channel1_IRQHandler
default_handler DMA1_Channel2_IRQHandler
default_handler DMA1_Channel3_IRQHandler
default_handler DMA1_Channel4_IRQHandler
default_handler DMA1_Channel5_IRQHandler
default_handler DMA1_Channel6_IRQHandler
default_handler DMA1_Channel7_IRQHandler
default_handler ADC1_2_IRQHandler
default_handler USB_HP_CAN1_TX_IRQHandler
default_handler USB_LP_CAN1_RX0_IRQHandler
default_handler CAN1_RX1_IRQHandler
default_handler CAN1_SCE_IRQHandler
default_handler EXTI9_5_IRQHandler
default_handler TIM1_BRK_IRQHandler
default_handler TIM1_UP_IRQHandler
default_handler TIM1_TRG_COM_IRQHandler
default_handler TIM1_CC_IRQHandler
default_handler TIM2_IRQHandler
default_handler TIM3_IRQHandler
default_handler TIM4_IRQHandler
default_handler I2C1_EV_IRQHandler
default_handler I2C1_ER_IRQHandler
default_handler I2C2_EV_IRQHandler
default_handler I2C2_ER_IRQHandler
default_handler SPI1_IRQHandler
default_handler SPI2_IRQHandler
default_handler USART1_IRQHandler
default_handler USART2_IRQHandler
default_handler USART3_IRQHandler
default_handler EXTI15_10_IRQHandler
default_handler RTC_Alarm_IRQHandler
default_handler USBWakeUp_IRQHandler

.section .note.GNU-stack,"",%progbits
