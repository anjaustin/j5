.section .text
.global _start
_start:
    # Set stack pointer (top of ITIM RAM at 0x80004000)
    lui sp, 0x80004
    
    # Jump to main
    jal main
    
    # Loop forever if main returns
1:  j 1b
