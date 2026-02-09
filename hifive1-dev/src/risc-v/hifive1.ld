/*
 * hifive1.ld - Linker script for HiFive1 Rev B
 * 
 * Memory map:
 *   Flash (IS25LP032): 0x20000000 - 0x20400000 (4MB)
 *   RAM:               0x80000000 - 0x80004000 (16KB)
 */

MEMORY
{
    /* Flash - 4MB at 0x20000000 */
    flash (rx) : ORIGIN = 0x20000000, LENGTH = 4M
    
    /* RAM - 16KB at 0x80000000 */
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 16K
}

SECTIONS
{
    /* Program code and read-only data goes to flash */
    .text : {
        *(.text.init)
        *(.text.startup)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > flash
    
    /* Initialized data - loaded from flash to RAM */
    .data : {
        _data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _data_end = .;
    } > ram AT > flash
    
    _data_load = LOADADDR(.data);
    
    /* Uninitialized data (BSS) - zeroed at startup */
    .bss : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram
    
    /* Stack top (grows down from end of RAM) */
    _stack_top = 0x80004000;
    
    /* End of used RAM */
    _end = .;
}

/* Entry point */
ENTRY(_start)
