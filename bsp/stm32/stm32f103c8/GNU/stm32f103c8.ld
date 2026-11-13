/* SPDX-License-Identifier: MIT
 * STM32F103C8: documented 64 KiB Flash and 20 KiB SRAM, no bootloader offset.
 */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE cortex-m-sections.ld

ASSERT(SIZEOF(.isr_vector) == 59 * 4, "STM32F103 medium-density vector size mismatch")
