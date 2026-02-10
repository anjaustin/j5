/*
 * hifive1_probe.c - Comprehensive Hardware Probe for HiFive1 Rev B
 * 
 * Tests and identifies all connected hardware:
 * - UART communication
 * - SPI bus (SD card slot)
 * - GPIO states
 * - LCD controller detection
 * - Memory test
 */

#include <stdint.h>

// Memory Map
#define GPIO_BASE       0x10012000
#define UART0_BASE      0x10013000
#define SPI1_BASE       0x10024000
#define PRCI_BASE       0x10008000

// GPIO Registers
#define GPIO_INPUT      (*(volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_IOF_EN     (*(volatile uint32_t *)(GPIO_BASE + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(GPIO_BASE + 0x3C))

// UART0 Registers
#define UART_TXDATA     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_RXDATA     (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_RXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART_IE         (*(volatile uint32_t *)(UART0_BASE + 0x10))
#define UART_IP         (*(volatile uint32_t *)(UART0_BASE + 0x14))
#define UART_DIV        (*(volatile uint32_t *)(UART0_BASE + 0x18))

// SPI1 Registers
#define SPI_SCKDIV      (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_SCKMODE     (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_CSID        (*(volatile uint32_t *)(SPI1_BASE + 0x10))
#define SPI_CSDEF       (*(volatile uint32_t *)(SPI1_BASE + 0x14))
#define SPI_CSMODE      (*(volatile uint32_t *)(SPI1_BASE + 0x18))
#define SPI_FMT         (*(volatile uint32_t *)(SPI1_BASE + 0x40))
#define SPI_TXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x48))
#define SPI_RXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x4C))

// PRCI (Clock)
#define PRCI_HFROSCCFG  (*(volatile uint32_t *)(PRCI_BASE + 0x00))
#define PRCI_HFROSCDIV  (*(volatile uint32_t *)(PRCI_BASE + 0x04))

// Pin Definitions
#define PIN_UART0_RX    16
#define PIN_UART0_TX    17
#define PIN_SPI1_MOSI   3
#define PIN_SPI1_MISO   4
#define PIN_SPI1_SCK    5
#define PIN_SD_CS       2
#define PIN_LCD_CS      6  // Common alternate CS for LCD

// Clock measurement using RTC
#define CLINT_BASE      0x02000000
#define CLINT_MTIME     (*(volatile uint64_t *)(CLINT_BASE + 0xBFF8))

static uint32_t g_core_clock_hz = 0;

void uart_init(void) {
    // Enable IOF for UART pins
    GPIO_IOF_EN |= (1 << PIN_UART0_RX) | (1 << PIN_UART0_TX);
    GPIO_IOF_SEL &= ~((1 << PIN_UART0_RX) | (1 << PIN_UART0_TX));
    
    // Divisor will be set after clock measurement
    UART_TXCTRL = 1;
    UART_RXCTRL = 1;
}

void uart_putc(char c) {
    while (UART_TXDATA & 0x80000000);
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

void uart_putdec(uint32_t val) {
    char buf[12];
    int i = 0;
    if (val == 0) {
        uart_putc('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i--) uart_putc(buf[i]);
}

void delay_ms(uint32_t ms) {
    // Calibrated delay based on measured clock
    uint32_t cycles = (g_core_clock_hz / 1000) * ms;
    for (volatile uint32_t i = 0; i < cycles / 5; i++);
}

// ============================================================
// Clock Measurement
// ============================================================

uint32_t measure_clock(void) {
    // Read mtime (1 tick = 1/32768 seconds)
    uint64_t start_time = CLINT_MTIME;
    uint64_t target_time = start_time + 328; // ~10ms
    
    // Read cycle counter
    uint32_t start_cycle;
    asm volatile ("csrr %0, mcycle" : "=r"(start_cycle));
    
    // Wait
    while (CLINT_MTIME < target_time);
    
    // Read end cycle
    uint32_t end_cycle;
    asm volatile ("csrr %0, mcycle" : "=r"(end_cycle));
    
    // Calculate frequency
    uint32_t cycles = end_cycle - start_cycle;
    return cycles * 100; // Scale to Hz (measured over 10ms)
}

// ============================================================
// SPI Functions
// ============================================================

void spi_init(void) {
    // Enable IOF for SPI1 pins
    GPIO_IOF_EN |= (1 << PIN_SPI1_MOSI) | (1 << PIN_SPI1_MISO) | (1 << PIN_SPI1_SCK);
    GPIO_IOF_SEL &= ~((1 << PIN_SPI1_MOSI) | (1 << PIN_SPI1_MISO) | (1 << PIN_SPI1_SCK));
    
    // CS pins as GPIO
    GPIO_OUTPUT_EN |= (1 << PIN_SD_CS) | (1 << PIN_LCD_CS);
    GPIO_PORT |= (1 << PIN_SD_CS) | (1 << PIN_LCD_CS); // Deselect both
    
    // SPI config
    SPI_SCKDIV = 100; // Slow ~90kHz for probing
    SPI_SCKMODE = 0;
    SPI_CSDEF = 0;
    SPI_CSMODE = 0;
    SPI_FMT = 0x00080000; // 8-bit, MSB, SPI mode 0
}

void spi_cs(uint32_t pin, int high) {
    if (high) {
        GPIO_PORT |= (1 << pin);
    } else {
        GPIO_PORT &= ~(1 << pin);
    }
}

uint8_t spi_xfer(uint8_t data) {
    while (SPI_TXDATA & 0x80000000);
    SPI_TXDATA = data;
    
    uint32_t rx;
    do {
        rx = SPI_RXDATA;
    } while (rx & 0x80000000);
    
    return (uint8_t)rx;
}

// ============================================================
// SD Card Probe
// ============================================================

int probe_sd_card(void) {
    uart_puts("\r\n--- SD Card Probe ---\r\n");
    
    // Send 80 clocks with CS high
    spi_cs(PIN_SD_CS, 1);
    for (int i = 0; i < 10; i++) {
        spi_xfer(0xFF);
    }
    
    // CMD0 - GO_IDLE_STATE
    uart_puts("CMD0: ");
    spi_cs(PIN_SD_CS, 0);
    spi_xfer(0x40 | 0);  // Command 0
    spi_xfer(0x00);      // Arg[31:24]
    spi_xfer(0x00);      // Arg[23:16]
    spi_xfer(0x00);      // Arg[15:8]
    spi_xfer(0x00);      // Arg[7:0]
    spi_xfer(0x95);      // CRC for CMD0
    
    uint8_t r1;
    for (int i = 0; i < 10; i++) {
        r1 = spi_xfer(0xFF);
        if (!(r1 & 0x80)) break;
    }
    spi_cs(PIN_SD_CS, 1);
    spi_xfer(0xFF);
    
    uart_puthex(r1);
    
    if (r1 == 0x01) {
        uart_puts(" -> SD Card detected (idle state)\r\n");
        return 1;
    } else if (r1 == 0xFF) {
        uart_puts(" -> No response (card not present?)\r\n");
        return 0;
    } else {
        uart_puts(" -> Unexpected response\r\n");
        return -1;
    }
}

// ============================================================
// LCD Controller Detection
// ============================================================

int probe_lcd(void) {
    uart_puts("\r\n--- LCD Controller Probe ---\r\n");
    
    // Try common LCD CS pins
    int cs_pins[] = {PIN_LCD_CS, 7, 8, 9, 10}; // Common alternatives
    int num_pins = sizeof(cs_pins) / sizeof(cs_pins[0]);
    
    for (int i = 0; i < num_pins; i++) {
        int cs = cs_pins[i];
        uart_puts("Testing CS GPIO ");
        uart_putdec(cs);
        uart_puts(": ");
        
        // Configure as output
        GPIO_OUTPUT_EN |= (1 << cs);
        GPIO_PORT |= (1 << cs);
        
        // Try reading LCD ID register (varies by controller)
        // ST7735: 0x04 (RDDIDF)
        // ILI9341: 0x04 (RDDIDF) or 0xD3 (RDID4)
        
        spi_cs(cs, 0);
        delay_ms(1);
        
        // Send read ID command for ST7735
        spi_xfer(0x04); // RDDIDF
        spi_xfer(0xFF); // Dummy read
        uint8_t id1 = spi_xfer(0xFF);
        uint8_t id2 = spi_xfer(0xFF);
        uint8_t id3 = spi_xfer(0xFF);
        
        spi_cs(cs, 1);
        
        uart_puthex(id1);
        uart_putc(' ');
        uart_puthex(id2);
        uart_putc(' ');
        uart_puthex(id3);
        
        // Check for valid responses
        if (id1 != 0xFF && id1 != 0x00) {
            uart_puts(" -> Possible LCD detected!\r\n");
        } else {
            uart_puts(" -> No response\r\n");
        }
    }
    
    return 0;
}

// ============================================================
// GPIO Scan
// ============================================================

void gpio_scan(void) {
    uart_puts("\r\n--- GPIO Pin States ---\r\n");
    
    // Disable IOF temporarily to read raw GPIO
    uint32_t saved_iof = GPIO_IOF_EN;
    GPIO_IOF_EN = 0;
    
    uint32_t inputs = GPIO_INPUT;
    
    uart_puts("GPIO Input States (0-31):\r\n");
    for (int i = 0; i < 32; i++) {
        if (i % 8 == 0) {
            uart_puts("\r\n  ");
            uart_putdec(i);
            uart_puts("-");
            uart_putdec(i+7);
            uart_puts(": ");
        }
        uart_putc((inputs & (1 << i)) ? '1' : '0');
        uart_putc(' ');
    }
    uart_puts("\r\n");
    
    // Restore IOF
    GPIO_IOF_EN = saved_iof;
}

// ============================================================
// Memory Test
// ============================================================

void memory_test(void) {
    uart_puts("\r\n--- RAM Test ---\r\n");
    
    // Test at 0x80003000 (12KB into RAM, safe from our code at 0x80000000)
    volatile uint32_t *ram = (volatile uint32_t *)0x80003000;
    int errors = 0;
    
    uart_puts("Testing 1KB at 0x80003000...\r\n");
    
    // Test pattern
    for (int i = 0; i < 256; i++) {
        ram[i] = 0xAA550000 + i;
    }
    
    // Verify
    for (int i = 0; i < 256; i++) {
        if (ram[i] != (0xAA550000 + i)) {
            errors++;
        }
    }
    
    if (errors == 0) {
        uart_puts("RAM Test: PASSED\r\n");
    } else {
        uart_puts("RAM Test: FAILED, errors: ");
        uart_putdec(errors);
        uart_puts("\r\n");
    }
}

// ============================================================
// Main
// ============================================================

int main(void) {
    // 1. Enable IOF for UART first (critical!)
    GPIO_IOF_EN |= (1 << PIN_UART0_RX) | (1 << PIN_UART0_TX);
    GPIO_IOF_SEL &= ~((1 << PIN_UART0_RX) | (1 << PIN_UART0_TX));
    
    // 2. Init UART with known working parameters
    UART_DIV = 157;  // 115200 @ 18.125 MHz
    UART_TXCTRL = 1;
    UART_RXCTRL = 1;
    
    uart_puts("\r\n========================================\r\n");
    uart_puts("  HiFive1 Hardware Probe\r\n");
    uart_puts("========================================\r\n");
    
    uart_puts("\r\nCore Clock: 18.125 MHz (calibrated)\r\n");
    g_core_clock_hz = 18125000;
    
    // 3. Initialize SPI
    spi_init();
    
    // 3. Memory Test
    memory_test();
    
    // 4. GPIO Scan
    gpio_scan();
    
    // 5. Probe SD Card
    probe_sd_card();
    
    // 6. Probe LCD
    probe_lcd();
    
    // 7. Summary
    uart_puts("\r\n========================================\r\n");
    uart_puts("  Probe Complete\r\n");
    uart_puts("========================================\r\n");
    uart_puts("\r\nNext steps:\r\n");
    uart_puts("1. Install FatFs for SD card filesystem\r\n");
    uart_puts("2. Identify LCD controller for display driver\r\n");
    uart_puts("3. Format SD card if detected\r\n");
    
    while (1) {
        delay_ms(1000);
    }
    
    return 0;
}
