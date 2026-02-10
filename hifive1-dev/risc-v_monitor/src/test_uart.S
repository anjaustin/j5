.section .text
.global _start
_start:
    # 1. Configure GPIO 16, 17
    lui t0, 0x10012
    lui t1, 0x30        # 0x30000
    lw t2, 0x38(t0)     # IOF_EN
    or t2, t2, t1
    sw t2, 0x38(t0)
    lw t2, 0x3C(t0)     # IOF_SEL
    not t1, t1
    and t2, t2, t1
    sw t2, 0x3C(t0)
    
    # 2. Configure UART0
    lui t0, 0x10013
    
    # Divisor 157 (18.125 MHz)
    addi t1, zero, 157
    sw t1, 24(t0)
    
    # Enable TX
    addi t1, zero, 1
    sw t1, 8(t0)
    
loop:
    li t2, 'H'
    call uart_send
    li t2, 'i'
    call uart_send
    li t2, '!'
    call uart_send
    li t2, '\n'
    call uart_send
    
    li t3, 1000000
delay:
    addi t3, t3, -1
    bnez t3, delay
    j loop

uart_send:
    lui t0, 0x10013
wait_tx:
    lw t1, 0(t0)
    bltz t1, wait_tx
    sw t2, 0(t0)
    ret
