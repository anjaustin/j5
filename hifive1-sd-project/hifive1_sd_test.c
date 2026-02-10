/*
 * hifive1_sd_test.c - SD Card Test for HiFive1 Rev B
 * 
 * Target: SiFive HiFive1 Rev B (FE310-G002)
 * Hardware: Arduino LCD Shield with microSD
 * 
 * Configuration:
 *   - Core Clock: 18.125 MHz (HFROSC)
 *   - Console: UART0 (115200 baud)
 *   - SD Card: SPI1
 * 
 * Pinout:
 *   - UART0 TX: GPIO 17 (IOF0)
 *   - UART0 RX: GPIO 16 (IOF0)
 *   - SPI1 MOSI: GPIO 3 (IOF0) -> D11
 *   - SPI1 MISO: GPIO 4 (IOF0) -> D12
 *   - SPI1 SCK:  GPIO 5 (IOF0) -> D13
 *   - SD CS:     GPIO 2 (GPIO) -> D10
 */

#include <stdint.h>

// ============================================================
// Memory Map & Registers
// ============================================================

#define GPIO_BASE       0x10012000
#define UART0_BASE      0x10013000
#define SPI1_BASE       0x10024000

// GPIO
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_IOF_EN     (*(volatile uint32_t *)(GPIO_BASE + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(GPIO_BASE + 0x3C))

// UART0
#define UART_TXDATA     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_RXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART_DIV        (*(volatile uint32_t *)(UART0_BASE + 0x18))

// SPI1
#define SPI_SCKDIV      (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_SCKMODE     (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_CSID        (*(volatile uint32_t *)(SPI1_BASE + 0x10))
#define SPI_CSDEF       (*(volatile uint32_t *)(SPI1_BASE + 0x14))
#define SPI_CSMODE      (*(volatile uint32_t *)(SPI1_BASE + 0x18))
#define SPI_FMT         (*(volatile uint32_t *)(SPI1_BASE + 0x40))
#define SPI_TXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x48))
#define SPI_RXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x4C))

// Pins
#define PIN_UART0_RX    16
#define PIN_UART0_TX    17
#define PIN_SPI1_MOSI   3
#define PIN_SPI1_MISO   4
#define PIN_SPI1_SCK    5
#define PIN_SD_CS       2   // D10 on HiFive1 Rev B

// SD Commands
#define SD_CMD0         0
#define SD_CMD8         8
#define SD_CMD55        55
#define SD_ACMD41       41
#define SD_CMD58        58

// ============================================================
// Low-Level Helper Functions
// ============================================================

void gpio_init(void) {
    // 1. Enable IOF0 for UART0 (16, 17) and SPI1 (3, 4, 5)
    // Mask = (1<<16)|(1<<17)|(1<<3)|(1<<4)|(1<<5)
    // 0x30000 | 0x38 = 0x30038
    uint32_t iof_mask = (1 << 16) | (1 << 17) | (1 << 3) | (1 << 4) | (1 << 5);
    
    GPIO_IOF_EN |= iof_mask;
    GPIO_IOF_SEL &= ~iof_mask; // Select IOF0
    
    // 2. Configure CS (GPIO 2) as Output
    GPIO_OUTPUT_EN |= (1 << PIN_SD_CS);
    GPIO_PORT |= (1 << PIN_SD_CS); // Default High (Deselect)
}

void uart_init(void) {
    // 18.125 MHz / 115200 - 1 = 157
    UART_DIV = 157;
    UART_TXCTRL = 1; // TXEN
    UART_RXCTRL = 1; // RXEN
}

void uart_putc(char c) {
    while (UART_TXDATA & 0x80000000); // Wait for Full flag
    UART_TXDATA = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void uart_puthex(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

void delay_ms(volatile uint32_t ms) {
    // 18125 cycles per ms approx. Loop overhead ~5? 3600 iters.
    for (volatile uint32_t i = 0; i < ms * 3600; i++);
}

// ============================================================
// SPI Functions
// ============================================================

void spi_init(void) {
    // SCKDIV: 18.125MHz / (2 * (div + 1)) = SPI_CLK
    // For init (<400kHz), target 200kHz.
    // 18.125M / 200k = 90. 90/2 = 45.
    SPI_SCKDIV = 45;
    
    SPI_SCKMODE = 0; // Mode 0
    SPI_CSDEF = 0;   // Hardware CS unused (we use GPIO)
    SPI_CSMODE = 0;  // Auto CS disabled
    
    // FMT: Protocol 0 (SPI), 8 bits, MSB
    SPI_FMT = 0x00080000;
}

void spi_set_fast(void) {
    // Switch to fast clock after init
    // Target 4 MHz?
    // 18M / 4M = 4.5 -> div 2
    SPI_SCKDIV = 2;
}

uint8_t spi_transfer(uint8_t data) {
    while (SPI_TXDATA & 0x80000000);
    SPI_TXDATA = data;
    
    uint32_t rx;
    do {
        rx = SPI_RXDATA;
    } while (rx & 0x80000000);
    
    return (uint8_t)rx;
}

void cs_low(void) {
    GPIO_PORT &= ~(1 << PIN_SD_CS);
}

void cs_high(void) {
    GPIO_PORT |= (1 << PIN_SD_CS);
}

// ============================================================
// SD Card Functions
// ============================================================

uint8_t sd_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t crc = 0xFF;
    if (cmd == SD_CMD0) crc = 0x95;
    if (cmd == SD_CMD8) crc = 0x87;
    
    spi_transfer(0x40 | cmd);
    spi_transfer((arg >> 24) & 0xFF);
    spi_transfer((arg >> 16) & 0xFF);
    spi_transfer((arg >> 8) & 0xFF);
    spi_transfer(arg & 0xFF);
    spi_transfer(crc);
    
    uint8_t r1;
    for (int i = 0; i < 10; i++) {
        r1 = spi_transfer(0xFF);
        if (!(r1 & 0x80)) return r1; // Valid response starts with 0 bit
    }
    return r1;
}

int sd_init(void) {
    uart_puts("Sending 80 clocks...\r\n");
    cs_high();
    for (int i = 0; i < 10; i++) spi_transfer(0xFF);
    
    uart_puts("CMD0... ");
    cs_low();
    uint8_t r1 = sd_cmd(SD_CMD0, 0);
    cs_high();
    spi_transfer(0xFF);
    uart_puthex(r1);
    uart_puts("\r\n");
    
    if (r1 != 0x01) return -1;
    
    uart_puts("CMD8... ");
    cs_low();
    r1 = sd_cmd(SD_CMD8, 0x1AA);
    uint32_t r7 = 0;
    r7 |= (uint32_t)spi_transfer(0xFF) << 24;
    r7 |= (uint32_t)spi_transfer(0xFF) << 16;
    r7 |= (uint32_t)spi_transfer(0xFF) << 8;
    r7 |= (uint32_t)spi_transfer(0xFF);
    cs_high();
    spi_transfer(0xFF);
    uart_puthex(r1);
    uart_puts(" R7: ");
    uart_puthex(r7);
    uart_puts("\r\n");
    
    if (r1 == 0x01 && (r7 & 0xFF) == 0xAA) {
        uart_puts("SDv2 detected. Initializing...\r\n");
        int retries = 1000;
        while (retries--) {
            cs_low();
            sd_cmd(SD_CMD55, 0);
            cs_high();
            spi_transfer(0xFF);
            
            cs_low();
            r1 = sd_cmd(SD_ACMD41, 0x40000000); // HCS = 1
            cs_high();
            spi_transfer(0xFF);
            
            if (r1 == 0x00) break;
            delay_ms(10);
        }
        
        if (r1 != 0x00) {
            uart_puts("ACMD41 timeout. R1: ");
            uart_puthex(r1);
            uart_puts("\r\n");
            return -2;
        }
        
        uart_puts("SD Card Ready!\r\n");
        
        // Read OCR
        cs_low();
        sd_cmd(SD_CMD58, 0);
        uint32_t ocr = 0;
        ocr |= (uint32_t)spi_transfer(0xFF) << 24;
        ocr |= (uint32_t)spi_transfer(0xFF) << 16;
        ocr |= (uint32_t)spi_transfer(0xFF) << 8;
        ocr |= (uint32_t)spi_transfer(0xFF);
        cs_high();
        spi_transfer(0xFF);
        
        uart_puts("OCR: ");
        uart_puthex(ocr);
        uart_puts(ocr & 0x40000000 ? " (CCS: Block Addr)\r\n" : " (CCS: Byte Addr)\r\n");
        
        return 0;
    }
    
    return -3;
}

// ============================================================
// Main
// ============================================================

int main(void) {
    gpio_init();
    uart_init();
    
    uart_puts("\r\n--- SD Card Probe ---\r\n");
    
    spi_init();
    
    if (sd_init() == 0) {
        uart_puts("SUCCESS: SD Card detected and initialized.\r\n");
    } else {
        uart_puts("FAILURE: Could not init SD Card.\r\n");
    }
    
    while(1);
}
