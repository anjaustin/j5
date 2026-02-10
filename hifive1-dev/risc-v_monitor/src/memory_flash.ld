OUTPUT_ARCH("riscv")
ENTRY(_start)
MEMORY { flash (rx) : ORIGIN = 0x20010000, LENGTH = 4M }
SECTIONS { .text : { *(.text*) } > flash .rodata : { *(.rodata*) } > flash /DISCARD/ : { *(.data*) *(.bss*) } }