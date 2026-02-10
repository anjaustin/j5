.section .text
.global _start
_start:
    # CLINT mtime address: 0x0200BFF8
    # 0x0200BFF8 = 0x0200C000 - 8
    lui t0, 0x0200C
    addi t0, t0, -8     # t0 points to mtime (low 32 bits)
    
    # 1. Sync with mtime edge (wait for value to change)
    lw t1, 0(t0)
sync_loop:
    lw t2, 0(t0)
    beq t1, t2, sync_loop
    
    # Start counting
    csrr t3, mcycle
    
    # 2. Wait for 327 ticks (approx 10ms at 32.768kHz)
    # 32768 / 100 = 327.68
    addi t4, t2, 328
    
wait_loop:
    lw t2, 0(t0)
    # Simple unsigned comparison for wait (assuming no overflow wrap in 10ms)
    bltu t2, t4, wait_loop
    
    # Stop counting
    csrr t5, mcycle
    
    # 3. Calculate diff
    sub s0, t5, t3      # s0 = cycles in 10ms
    
    # 4. Multiply by 100 to get Hz
    li t6, 100
    mul s0, s0, t6      # s0 = Clock Hz
    
done:
    j done
