OUTPUT_ARCH("riscv")
ENTRY(main)

MEMORY {
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 16K
}

SECTIONS {
  .text : {
    *(.text*)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  } > ram

  .rodata : {
    *(.rodata*)
    *(.gnu.linkonce.r.*)
  } > ram

  .data : {
    *(.data*)
    *(.gnu.linkonce.d.*)
  } > ram

  .bss : {
    *(.bss*)
    *(.gnu.linkonce.b.*)
  } > ram
}
