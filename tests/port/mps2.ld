/* Cortex-M3 core-only test image for QEMU MPS2-AN385, not STM32 firmware. */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx) : ORIGIN = 0, LENGTH = 64K
    RAM (rw) : ORIGIN = 0x20000000, LENGTH = 20K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : { *(.text*) *(.rodata*) KEEP(*(.init)) } > FLASH
    .preinit_array :
    {
        __preinit_array_start = .; KEEP(*(.preinit_array*)) __preinit_array_end = .;
    } > FLASH
    .init_array :
    {
        __init_array_start = .; KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH
    .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
    _sidata = LOADADDR(.data);
    .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
    /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}
ASSERT(_ebss <= _estack - 1024, "Test data overlaps MSP stack")
