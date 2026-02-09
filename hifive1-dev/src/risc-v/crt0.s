.section .text.init

# Entry point
.global _start
_start:
    # Initialize stack pointer
    la sp, _stack_top
    
    # Initialize data section (copy from flash to RAM)
    la t0, _data_load
    la t1, _data_start
    la t2, _data_end
    
copy_data:
    bgeu t1, t2, data_done
    lw t3, 0(t0)
    sw t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    j copy_data
    
data_done:
    # Clear BSS section
    la t0, _bss_start
    la t1, _bss_end
    
clear_bss:
    bgeu t0, t1, bss_done
    sw zero, 0(t0)
    addi t0, t0, 4
    j clear_bss
    
bss_done:
    # Call main
    call main
    
    # If main returns, loop forever
end:
    j end
