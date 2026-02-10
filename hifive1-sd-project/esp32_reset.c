#include <stdint.h>

#define GPIO_BASE    0x10012000
#define UART1_BASE   0x10013000

#define GPIO_OUTPUT_EN  (GPIO_BASE + 0x08)
#define GPIO_OUTPUT_VAL (GPIO_BASE + 0x0C)
#define UART_TXDATA     (UART1_BASE + 0x00)
#define UART_RXDATA     (UART1_BASE + 0x04)
#define UART_TXCTRL     (UART1_BASE + 0x08)
#define UART_RXCTRL     (UART1_BASE + 0x0C)
#define UART_STATUS     (UART1_BASE + 0x18)

#define ESP32_GPIO0_PIN  2
#define ESP32_RESET_PIN  3

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
    puts("ESP32 Reset Utility\r\n");

    gpio_set_output(ESP32_RESET_PIN, 1);
    gpio_set_output(ESP32_GPIO0_PIN, 1);
    delay(100000);

    puts("GPIO0 LOW for download mode\r\n");
    gpio_set_output(ESP32_GPIO0_PIN, 0);
    delay(1000);

    puts("Resetting ESP32...\r\n");
    gpio_set_output(ESP32_RESET_PIN, 0);
    delay(1000);
    gpio_set_output(ESP32_RESET_PIN, 1);
    delay(100000);

    puts("Sync bytes sent.\r\n");
    for (int i = 0; i < 20; i++) {
        uart1_putchar(0x00);
        delay(1000);
    }

    puts("Done.\r\n");
    return 0;
}
