/* SPDX-License-Identifier: MIT
 * IRQ order follows the STM32F439xx interrupt table in ST RM0090.
 */
.syntax unified
.thumb
.eabi_attribute 24, 1
.eabi_attribute 25, 1

#include "../../../libcpu/arm/common/startup_gcc.inc"

.section .isr_vector,"a",%progbits
.balign 512
.global g_pfnVectors, __Vectors
.type g_pfnVectors,%object

.macro irq name
    .word \name
    default_handler \name
.endm

g_pfnVectors:
__Vectors:
    .word _estack, Reset_Handler
    irq NMI_Handler
    irq HardFault_Handler
    irq MemManage_Handler
    irq BusFault_Handler
    irq UsageFault_Handler
    .word 0, 0, 0, 0
    irq SVC_Handler
    irq DebugMon_Handler
    .word 0, PendSV_Handler, SysTick_Handler

    irq WWDG_IRQHandler
    irq PVD_IRQHandler
    irq TAMP_STAMP_IRQHandler
    irq RTC_WKUP_IRQHandler
    irq FLASH_IRQHandler
    irq RCC_IRQHandler
    irq EXTI0_IRQHandler
    irq EXTI1_IRQHandler
    irq EXTI2_IRQHandler
    irq EXTI3_IRQHandler
    irq EXTI4_IRQHandler
    irq DMA1_Stream0_IRQHandler
    irq DMA1_Stream1_IRQHandler
    irq DMA1_Stream2_IRQHandler
    irq DMA1_Stream3_IRQHandler
    irq DMA1_Stream4_IRQHandler
    irq DMA1_Stream5_IRQHandler
    irq DMA1_Stream6_IRQHandler
    irq ADC_IRQHandler
    irq CAN1_TX_IRQHandler
    irq CAN1_RX0_IRQHandler
    irq CAN1_RX1_IRQHandler
    irq CAN1_SCE_IRQHandler
    irq EXTI9_5_IRQHandler
    irq TIM1_BRK_TIM9_IRQHandler
    irq TIM1_UP_TIM10_IRQHandler
    irq TIM1_TRG_COM_TIM11_IRQHandler
    irq TIM1_CC_IRQHandler
    irq TIM2_IRQHandler
    irq TIM3_IRQHandler
    irq TIM4_IRQHandler
    irq I2C1_EV_IRQHandler
    irq I2C1_ER_IRQHandler
    irq I2C2_EV_IRQHandler
    irq I2C2_ER_IRQHandler
    irq SPI1_IRQHandler
    irq SPI2_IRQHandler
    irq USART1_IRQHandler
    irq USART2_IRQHandler
    irq USART3_IRQHandler
    irq EXTI15_10_IRQHandler
    irq RTC_Alarm_IRQHandler
    irq OTG_FS_WKUP_IRQHandler
    irq TIM8_BRK_TIM12_IRQHandler
    irq TIM8_UP_TIM13_IRQHandler
    irq TIM8_TRG_COM_TIM14_IRQHandler
    irq TIM8_CC_IRQHandler
    irq DMA1_Stream7_IRQHandler
    irq FMC_IRQHandler
    irq SDIO_IRQHandler
    irq TIM5_IRQHandler
    irq SPI3_IRQHandler
    irq UART4_IRQHandler
    irq UART5_IRQHandler
    irq TIM6_DAC_IRQHandler
    irq TIM7_IRQHandler
    irq DMA2_Stream0_IRQHandler
    irq DMA2_Stream1_IRQHandler
    irq DMA2_Stream2_IRQHandler
    irq DMA2_Stream3_IRQHandler
    irq DMA2_Stream4_IRQHandler
    irq ETH_IRQHandler
    irq ETH_WKUP_IRQHandler
    irq CAN2_TX_IRQHandler
    irq CAN2_RX0_IRQHandler
    irq CAN2_RX1_IRQHandler
    irq CAN2_SCE_IRQHandler
    irq OTG_FS_IRQHandler
    irq DMA2_Stream5_IRQHandler
    irq DMA2_Stream6_IRQHandler
    irq DMA2_Stream7_IRQHandler
    irq USART6_IRQHandler
    irq I2C3_EV_IRQHandler
    irq I2C3_ER_IRQHandler
    irq OTG_HS_EP1_OUT_IRQHandler
    irq OTG_HS_EP1_IN_IRQHandler
    irq OTG_HS_WKUP_IRQHandler
    irq OTG_HS_IRQHandler
    irq DCMI_IRQHandler
    irq CRYP_IRQHandler
    irq HASH_RNG_IRQHandler
    irq FPU_IRQHandler
    irq UART7_IRQHandler
    irq UART8_IRQHandler
    irq SPI4_IRQHandler
    irq SPI5_IRQHandler
    irq SPI6_IRQHandler
    irq SAI1_IRQHandler
    irq LTDC_IRQHandler
    irq LTDC_ER_IRQHandler
    irq DMA2D_IRQHandler
.size g_pfnVectors, .-g_pfnVectors

.section .note.GNU-stack,"",%progbits
