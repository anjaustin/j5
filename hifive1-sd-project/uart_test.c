#include <stdint.h>

#define UART1_BASE   0x10013000
#define UART_TXDATA  (UART1_BASE + 0x00)
#define UART_RXDATA  (UART1_BASE + 0x04)
#define UART_TXCTRL  (UART1_BASE + 0x08)
#define UART_RXCTRL  (UART1_BASE + 0x0C)
#define UART_STATUS  (UART1_BASE + 0x18)

void uart1_init(void) {
    *(volatile uint32_t *)(UART_TXCTRL) = 1;
    *(volatile uint32_t *)(UART_RXCTRL) = 1;
}

void uart1_putchar(char c) {
    while (*(volatile uint32_t *)(UART_STATUS) & 0x08);
    *(volatile uint32_t *)(UART_TXDATA) = c;
}

void delay(uint32_t count) {
    volatile uint32_t i;
    for (i = 0; i < count; i++);
}

void puts(const char *s) {
    while (*s) uart1_putchar(*s++);
}

int main(void) {
    uart1_init();
    puts("Hello from RISC-V!\r\n");
    return 0;
}
