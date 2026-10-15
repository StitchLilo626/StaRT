/* SPDX-License-Identifier: MIT
 * STM32F103C8: documented 64 KiB Flash and 20 KiB SRAM, no bootloader offset.
 */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}

__StackSize = 0x400;
_estack = ORIGIN(RAM) + LENGTH(RAM);
__StackTop = _estack;
__StackLimit = __StackTop - __StackSize;

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(512);
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH
    _sidata = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    .noinit (NOLOAD) :
    {
        *(.noinit .noinit.*)
        . = ALIGN(8);
        _end = .;
        PROVIDE(end = .);
    } > RAM
    /* Reserve the MSP stack at the top of RAM; no dynamic heap is provided. */
    .stack __StackLimit (NOLOAD) :
    {
        . += __StackSize;
    } > RAM
    /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}
ASSERT(ADDR(.isr_vector) == ORIGIN(FLASH), "Vector table must start at Flash base")
ASSERT(SIZEOF(.isr_vector) == 59 * 4, "STM32F103 medium-density vector size mismatch")
ASSERT(_end <= __StackLimit, "RAM data overlaps reserved MSP stack")
ASSERT((_estack & 7) == 0, "Initial MSP must be 8-byte aligned")
