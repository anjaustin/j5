MEMORY {
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 16K
}
SECTIONS {
    .text : {
        *(.text*)
        *(.rodata*)
    } > ram
    .data : { *(.data*) } > ram
    .bss : { *(.bss*) } > ram
}
ENTRY(_start)
