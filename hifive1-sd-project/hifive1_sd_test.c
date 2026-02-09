/*
 * hifive1_sd_test.c - SD Card Test for HiFive1 Rev B with Arduino LCD Shield
 * 
 * This program initializes SPI and attempts to read an SD card
 * connected to the Arduino shield's microSD slot.
 * 
 * SPI pins on HiFive1 (Arduino compatible):
 *   - MOSI: GPIO 3 (Arduino D11)
 *   - MISO: GPIO 4 (Arduino D12)  
 *   - SCK:  GPIO 5 (Arduino D13)
 *   - CS:   GPIO 10 (Arduino D10) - typically used for SD card
 * 
 * UART pins for debug output:
 *   - TX: GPIO 17
 *   - RX: GPIO 16
 */

#include <stdint.h>

// HiFive1 memory-mapped register addresses
#define GPIO_BASE       0x10012000
#define SPI0_BASE       0x10014000
#define UART0_BASE      0x10013000

// GPIO registers
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_PUE        (*(volatile uint32_t *)(GPIO_BASE + 0x10))

// SPI registers  
#define SPI_SCKDIV      (*(volatile uint32_t *)(SPI0_BASE + 0x00))
#define SPI_SCKMODE     (*(volatile uint32_t *)(SPI0_BASE + 0x04))
#define SPI_CSID        (*(volatile uint32_t *)(SPI0_BASE + 0x10))
#define SPI_CSDEF       (*(volatile uint32_t *)(SPI0_BASE + 0x14))
#define SPI_CSMODE      (*(volatile uint32_t *)(SPI0_BASE + 0x18))
#define SPI_DELAY0      (*(volatile uint32_t *)(SPI0_BASE + 0x28))
#define SPI_DELAY1      (*(volatile uint32_t *)(SPI0_BASE + 0x2C))
#define SPI_FMT         (*(volatile uint32_t *)(SPI0_BASE + 0x40))
#define SPI_TXDATA      (*(volatile uint32_t *)(SPI0_BASE + 0x48))
#define SPI_RXDATA      (*(volatile uint32_t *)(SPI0_BASE + 0x4C))
#define SPI_TXMARK      (*(volatile uint32_t *)(SPI0_BASE + 0x50))
#define SPI_RXMARK      (*(volatile uint32_t *)(SPI0_BASE + 0x54))
#define SPI_FCTRL       (*(volatile uint32_t *)(SPI0_BASE + 0x60))
#define SPI_FFMT        (*(volatile uint32_t *)(SPI0_BASE + 0x64))

// UART registers
#define UART_TXFIFO     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_RXFIFO     (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_RXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART_IE         (*(volatile uint32_t *)(UART0_BASE + 0x10))
#define UART_IP         (*(volatile uint32_t *)(UART0_BASE + 0x14))
#define UART_DIV        (*(volatile uint32_t *)(UART0_BASE + 0x18))

// Pin definitions for Arduino shield
#define PIN_SPI_MOSI    3   // GPIO 3
#define PIN_SPI_MISO    4   // GPIO 4
#define PIN_SPI_SCK     5   // GPIO 5
#define PIN_SPI_CS      10  // GPIO 10 (Arduino D10)

// SD Card Commands
#define SD_CMD0         0   // GO_IDLE_STATE
#define SD_CMD8         8   // SEND_IF_COND
#define SD_CMD55        55  // APP_CMD
#define SD_ACMD41       41  // SD_SEND_OP_COND
#define SD_CMD58        58  // READ_OCR
#define SD_CMD17        17  // READ_SINGLE_BLOCK

// UART baud rate (115200 at core clock)
// Core clock is typically 16MHz or 256MHz depending on PLL config
// Assuming 16MHz for safety: divisor = 16000000 / 115200 = 138.88
#define UART_DIVISOR    139

void uart_init(void) {
    // Disable UART first
    UART_TXCTRL = 0;
    UART_RXCTRL = 0;
    
    // Set baud rate divisor
    UART_DIV = UART_DIVISOR;
    
    // Enable TX and RX
    UART_TXCTRL = 0x1;  // TX enable
    UART_RXCTRL = 0x1;  // RX enable
}

void uart_putc(char c) {
    // Wait until TX FIFO has space
    while (UART_TXFIFO & 0x80000000);
    UART_TXFIFO = c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_puthex(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

void delay_ms(volatile uint32_t ms) {
    // Rough delay at 16MHz
    // Each iteration ~5 cycles, so ~3.2M iterations per second
    for (volatile uint32_t i = 0; i < ms * 3200; i++);
}

void spi_init(void) {
    // Configure GPIO pins for SPI
    // Enable output for MOSI, SCK, CS
    GPIO_OUTPUT_EN |= (1 << PIN_SPI_MOSI) | (1 << PIN_SPI_SCK) | (1 << PIN_SPI_CS);
    // Enable pull-up for MISO
    GPIO_PUE |= (1 << PIN_SPI_MISO);
    
    // Set CS high initially
    GPIO_PORT |= (1 << PIN_SPI_CS);
    
    // Configure SPI
    // SCKDIV: Divide core clock for SPI clock
    // At 16MHz core, divide by 32 = 500kHz SPI (safe for SD init)
    SPI_SCKDIV = 31;
    
    // SCKMODE: Mode 0 (CPOL=0, CPHA=0)
    SPI_SCKMODE = 0;
    
    // CSDEF: CS default is high
    SPI_CSDEF = 0x1;
    
    // CSMODE: Auto CS mode
    SPI_CSMODE = 0;
    
    // DELAY0: CS to SCK delay (1/2 SPI clock period)
    SPI_DELAY0 = 0x00010001;
    
    // DELAY1: SCK to CS delay
    SPI_DELAY1 = 0x00000001;
    
    // FMT: Format - SPI, MSB first, 8 bits per frame
    // proto=0 (SPI), endian=0 (MSB), dir=0 (RX), len=8
    SPI_FMT = 0x00080000;
    
    // Disable SPI flash mode
    SPI_FCTRL = 0;
}

uint8_t spi_transfer(uint8_t data) {
    // Wait for TX FIFO ready
    while (SPI_TXDATA & 0x80000000);
    
    // Send data
    SPI_TXDATA = data;
    
    // Wait for RX data
    uint32_t rx;
    do {
        rx = SPI_RXDATA;
    } while (rx & 0x80000000);
    
    return (uint8_t)rx;
}

void spi_cs_low(void) {
    GPIO_PORT &= ~(1 << PIN_SPI_CS);
}

void spi_cs_high(void) {
    GPIO_PORT |= (1 << PIN_SPI_CS);
}

// SD Card functions
uint8_t sd_send_command(uint8_t cmd, uint32_t arg) {
    uint8_t crc = 0xFF;
    
    // Send command
    spi_transfer(0x40 | cmd);
    spi_transfer((arg >> 24) & 0xFF);
    spi_transfer((arg >> 16) & 0xFF);
    spi_transfer((arg >> 8) & 0xFF);
    spi_transfer(arg & 0xFF);
    
    // CRC (only CMD0 and CMD8 need valid CRC in SPI mode)
    if (cmd == SD_CMD0) crc = 0x95;
    else if (cmd == SD_CMD8) crc = 0x87;
    spi_transfer(crc);
    
    // Wait for response (up to 8 bytes)
    uint8_t response;
    int retries = 10;
    do {
        response = spi_transfer(0xFF);
    } while ((response & 0x80) && --retries);
    
    return response;
}

int sd_init(void) {
    uart_puts("SD Card Init Starting...\r\n");
    
    // Power up sequence - CS high, send 80+ clocks
    spi_cs_high();
    for (int i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }
    
    // Send CMD0 - GO_IDLE_STATE
    spi_cs_low();
    uint8_t r1 = sd_send_command(SD_CMD0, 0);
    spi_cs_high();
    spi_transfer(0xFF);  // Extra clock
    
    uart_puts("CMD0 Response: 0x");
    uart_puthex(r1);
    uart_puts("\r\n");
    
    if (r1 != 0x01) {
        uart_puts("SD Card not responding to CMD0\r\n");
        return -1;
    }
    
    uart_puts("SD Card in idle state\r\n");
    
    // Send CMD8 - SEND_IF_COND (check voltage range)
    spi_cs_low();
    r1 = sd_send_command(SD_CMD8, 0x1AA);
    uint8_t r7[4];
    if (r1 == 0x01) {
        // Card responded to CMD8 (SDv2+)
        r7[0] = spi_transfer(0xFF);
        r7[1] = spi_transfer(0xFF);
        r7[2] = spi_transfer(0xFF);
        r7[3] = spi_transfer(0xFF);
        spi_cs_high();
        spi_transfer(0xFF);
        
        uart_puts("CMD8 Response: 0x");
        uart_puthex(r1);
        uart_puts(" (SDv2+ card detected)\r\n");
        
        // Check voltage acceptance
        if ((r7[2] & 0x0F) != 0x01 || r7[3] != 0xAA) {
            uart_puts("Voltage range not supported\r\n");
            return -1;
        }
    } else {
        spi_cs_high();
        spi_transfer(0xFF);
        uart_puts("CMD8 Response: 0x");
        uart_puthex(r1);
        uart_puts(" (SDv1 or MMC card)\r\n");
    }
    
    // Send ACMD41 - SD_SEND_OP_COND
    int retries = 100;
    do {
        // CMD55 first
        spi_cs_low();
        r1 = sd_send_command(SD_CMD55, 0);
        spi_cs_high();
        spi_transfer(0xFF);
        
        if (r1 != 0x01) {
            uart_puts("CMD55 failed\r\n");
            return -1;
        }
        
        // Then ACMD41
        spi_cs_low();
        if (r1 == 0x01) {
            // SDv2+ - use HCS bit
            r1 = sd_send_command(SD_ACMD41, 0x40000000);
        } else {
            // SDv1
            r1 = sd_send_command(SD_ACMD41, 0);
        }
        spi_cs_high();
        spi_transfer(0xFF);
        
        delay_ms(10);
    } while (r1 != 0x00 && --retries);
    
    if (r1 != 0x00) {
        uart_puts("ACMD41 failed - card not ready\r\n");
        return -1;
    }
    
    uart_puts("SD Card initialized successfully!\r\n");
    return 0;
}

int main(void) {
    // Initialize UART for debug output
    uart_init();
    
    // Small delay for UART to stabilize
    delay_ms(100);
    
    // Print startup message
    uart_puts("\r\n========================================\r\n");
    uart_puts("  HiFive1 SD Card Test\r\n");
    uart_puts("  Arduino LCD Shield + microSD\r\n");
    uart_puts("========================================\r\n\r\n");
    
    // Initialize SPI
    uart_puts("Initializing SPI...\r\n");
    spi_init();
    uart_puts("SPI initialized at 500kHz\r\n\r\n");
    
    // Try to initialize SD card
    if (sd_init() == 0) {
        uart_puts("\r\n*** SD Card Ready! ***\r\n");
        uart_puts("You can now read/write blocks\r\n");
    } else {
        uart_puts("\r\n*** SD Card Init Failed ***\r\n");
        uart_puts("Check:\r\n");
        uart_puts("  - Card is inserted\r\n");
        uart_puts("  - Card is not write-protected\r\n");
        uart_puts("  - Shield is properly seated\r\n");
    }
    
    // Main loop - blink or wait
    uart_puts("\r\nEntering main loop...\r\n");
    
    volatile uint32_t counter = 0;
    while (1) {
        // Toggle LED or just wait
        delay_ms(1000);
        counter++;
        
        // Print heartbeat every 10 seconds
        if (counter % 10 == 0) {
            uart_puts("Heartbeat: ");
            uart_puthex(counter);
            uart_puts(" seconds\r\n");
        }
    }
    
    return 0;
}
