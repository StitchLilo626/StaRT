/* SPDX-License-Identifier: MIT
 * STM32F439ZI: 2 MiB Flash, 192 KiB contiguous AHB SRAM. CCM is separate.
 */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 192K
}

__StackSize = 0x800;
INCLUDE cortex-m-sections.ld

ASSERT(SIZEOF(.isr_vector) == 107 * 4, "STM32F439 vector size mismatch")
