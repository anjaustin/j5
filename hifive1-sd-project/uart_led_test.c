#include <stdint.h>

#define GPIO_BASE    0x10012000
#define UART1_BASE   0x10013000

#define GPIO_OUTPUT_EN  (GPIO_BASE + 0x08)
#define GPIO_OUTPUT_VAL (GPIO_BASE + 0x0C)
#define GPIO_IOF_EN     (GPIO_BASE + 0x38)
#define GPIO_IOF_SEL    (GPIO_BASE + 0x3C)

#define UART_TXDATA     (UART1_BASE + 0x00)
#define UART_RXDATA     (UART1_BASE + 0x04)
#define UART_TXCTRL     (UART1_BASE + 0x08)
#define UART_RXCTRL     (UART1_BASE + 0x0C)
#define UART_STATUS     (UART1_BASE + 0x18)

#define LED_PIN  5  // GPIO5 is the green LED

void uart1_init(void) {
    // Enable UART1 TX/RX IOF
    *(volatile uint32_t *)(GPIO_IOF_EN) |= (1 << 16) | (1 << 17);
    *(volatile uint32_t *)(GPIO_IOF_SEL) &= ~((1 << 16) | (1 << 17));
    
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

void gpio_set_output(uint32_t pin, uint32_t val) {
    uint32_t en = *(volatile uint32_t *)(GPIO_OUTPUT_EN);
    en |= (1 << pin);
    *(volatile uint32_t *)(GPIO_OUTPUT_EN) = en;

    uint32_t out = *(volatile uint32_t *)(GPIO_OUTPUT_VAL);
    if (val) out |= (1 << pin);
    else out &= ~(1 << pin);
    *(volatile uint32_t *)(GPIO_OUTPUT_VAL) = out;
}

void puts(const char *s) {
    while (*s) uart1_putchar(*s++);
}

int main(void) {
    uart1_init();

    // Configure LED as output
    gpio_set_output(LED_PIN, 0);

    // Blink LED and print
    while (1) {
        gpio_set_output(LED_PIN, 1);  // LED on
        delay(500000);
        puts("Hello from RISC-V!\r\n");
        gpio_set_output(LED_PIN, 0);  // LED off
        delay(500000);
    }
    return 0;
}
