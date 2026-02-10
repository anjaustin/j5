/**
 * simple_test.c - Minimal test to verify UART works
 */

#include <stdint.h>

#define UART0_BASE      0x10013000
#define UART_TXDATA     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_DIV        (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define GPIO_IOF_EN     (*(volatile uint32_t *)(0x10012000 + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(0x10012000 + 0x3C))

void uart_init(void) {
    GPIO_IOF_EN |= (1 << 16) | (1 << 17);
    GPIO_IOF_SEL &= ~((1 << 16) | (1 << 17));
    UART_DIV = 157;
    UART_TXCTRL = 1;
}

void uart_putc(char c) {
    while (UART_TXDATA & 0x80000000);
    UART_TXDATA = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

int main(void) {
    uart_init();
    
    uart_puts("\r\n*** FATFS TEST ***\r\n");
    uart_puts("Hello from RISC-V!\r\n");
    uart_puts("If you see this, UART is working.\r\n");
    
    while(1);
    return 0;
}
