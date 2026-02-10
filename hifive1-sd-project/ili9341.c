/**
 * ili9341.c - ILI9341 TFT LCD Driver Implementation
 */

#include "ili9341.h"

// Register base addresses
#define GPIO_BASE       0x10012000
#define SPI1_BASE       0x10024000

// GPIO registers
#define GPIO_INPUT      (*(volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_IOF_EN     (*(volatile uint32_t *)(GPIO_BASE + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(GPIO_BASE + 0x3C))

// SPI1 registers
#define SPI_SCKDIV      (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_SCKMODE     (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_FMT         (*(volatile uint32_t *)(SPI1_BASE + 0x40))
#define SPI_TXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x48))
#define SPI_RXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x4C))

// ILI9341 Commands
#define CMD_NOP         0x00
#define CMD_SWRESET     0x01
#define CMD_SLPIN       0x10
#define CMD_SLPOUT      0x11
#define CMD_PTLON       0x12
#define CMD_NORON       0x13
#define CMD_INVOFF      0x20
#define CMD_INVON       0x21
#define CMD_GAMMASET    0x26
#define CMD_DISPOFF     0x28
#define CMD_DISPON      0x29
#define CMD_CASET       0x2A
#define CMD_PASET       0x2B
#define CMD_RAMWR       0x2C
#define CMD_MADCTL      0x36
#define CMD_PIXFMT      0x3A
#define CMD_FRMCTR1     0xB1
#define CMD_FRMCTR2     0xB2
#define CMD_FRMCTR3     0xB3
#define CMD_INVCTR      0xB4
#define CMD_PWCTR1      0xC0
#define CMD_PWCTR2      0xC1
#define CMD_PWCTR3      0xC2
#define CMD_PWCTR4      0xC3
#define CMD_PWCTR5      0xC4
#define CMD_VMCTR1      0xC5
#define CMD_VMCTR2      0xC6

static uint16_t _width = ILI9341_WIDTH;
static uint16_t _height = ILI9341_HEIGHT;

// GPIO helper functions
static void gpio_set(int pin, int high) {
    if (high) {
        GPIO_PORT |= (1 << pin);
    } else {
        GPIO_PORT &= ~(1 << pin);
    }
}

static void gpio_output(int pin) {
    GPIO_OUTPUT_EN |= (1 << pin);
}

// SPI transfer
static uint8_t spi_xfer(uint8_t data) {
    while (SPI_TXDATA & 0x80000000);
    SPI_TXDATA = data;
    
    uint32_t rx;
    do {
        rx = SPI_RXDATA;
    } while (rx & 0x80000000);
    
    return (uint8_t)rx;
}

// Send command (DC low)
static void write_command(uint8_t cmd) {
    gpio_set(ILI9341_DC_PIN, 0);
    gpio_set(ILI9341_CS_PIN, 0);
    spi_xfer(cmd);
    gpio_set(ILI9341_CS_PIN, 1);
}

// Send data (DC high)
static void write_data(uint8_t data) {
    gpio_set(ILI9341_DC_PIN, 1);
    gpio_set(ILI9341_CS_PIN, 0);
    spi_xfer(data);
    gpio_set(ILI9341_CS_PIN, 1);
}

// Delay function
static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 3600; i++);
}

void ili9341_init(void) {
    // 1. Configure GPIO pins
    // SPI1 pins (MOSI=3, SCK=5) need IOF
    GPIO_IOF_EN |= (1 << ILI9341_MOSI_PIN) | (1 << ILI9341_SCK_PIN);
    GPIO_IOF_SEL &= ~((1 << ILI9341_MOSI_PIN) | (1 << ILI9341_SCK_PIN));
    
    // Control pins as GPIO outputs
    gpio_output(ILI9341_CS_PIN);
    gpio_output(ILI9341_DC_PIN);
    gpio_output(ILI9341_RST_PIN);
    
    // Set initial states
    gpio_set(ILI9341_CS_PIN, 1);    // CS high (inactive)
    gpio_set(ILI9341_DC_PIN, 1);    // DC high (data mode)
    gpio_set(ILI9341_RST_PIN, 1);   // Reset high (not in reset)
    
    // 2. Configure SPI1
    // Start with slow speed for init (250kHz)
    // 18.125MHz / 250kHz = 72.5, so div = 71
    SPI_SCKDIV = 71;
    SPI_SCKMODE = 0;  // Mode 0 (CPOL=0, CPHA=0)
    SPI_FMT = 0x00080000;  // 8-bit, MSB first, SPI
    
    // 3. Hardware reset
    gpio_set(ILI9341_RST_PIN, 1);
    delay_ms(5);
    gpio_set(ILI9341_RST_PIN, 0);
    delay_ms(20);
    gpio_set(ILI9341_RST_PIN, 1);
    delay_ms(150);
    
    // 4. Initialization sequence
    write_command(CMD_SWRESET);     // Software reset
    delay_ms(200);
    
    write_command(CMD_SLPOUT);      // Exit sleep
    delay_ms(120);
    
    // Power Control A
    write_command(0xCB);
    write_data(0x39);
    write_data(0x2C);
    write_data(0x00);
    write_data(0x34);
    write_data(0x02);
    
    // Power Control B
    write_command(0xCF);
    write_data(0x00);
    write_data(0xC1);
    write_data(0x30);
    
    // Driver timing control A
    write_command(0xE8);
    write_data(0x85);
    write_data(0x00);
    write_data(0x78);
    
    // Driver timing control B
    write_command(0xEA);
    write_data(0x00);
    write_data(0x00);
    
    // Power on sequence control
    write_command(0xED);
    write_data(0x64);
    write_data(0x03);
    write_data(0x12);
    write_data(0x81);
    
    // Pump ratio control
    write_command(0xF7);
    write_data(0x20);
    
    // Power Control 1
    write_command(CMD_PWCTR1);
    write_data(0x23);  // VRH[5:0] = 4.6V
    
    // Power Control 2
    write_command(CMD_PWCTR2);
    write_data(0x10);  // SAP[2:0];BT[3:0] = 0x10
    
    // VCOM Control 1
    write_command(CMD_VMCTR1);
    write_data(0x3E);  // VCOMH = 4.25V
    write_data(0x28);  // VCOML = -1.5V
    
    // VCOM Control 2
    write_command(CMD_VMCTR2);
    write_data(0x86);  // VMF[6:0] = 0x86
    
    // Memory Access Control
    write_command(CMD_MADCTL);
    write_data(0x48);  // MX = 1, BGR = 1
    
    // Pixel Format
    write_command(CMD_PIXFMT);
    write_data(0x55);  // 16-bit/pixel (RGB565)
    
    // Frame Rate Control
    write_command(CMD_FRMCTR1);
    write_data(0x00);  // DIVA = fosc
    write_data(0x18);  // RTNA = 24 clocks per line (60Hz)
    
    // Display Function Control
    write_command(0xB6);
    write_data(0x08);
    write_data(0x82);
    write_data(0x27);
    
    // 3 Gamma Function Disable
    write_command(0xF2);
    write_data(0x00);
    
    // Gamma Set
    write_command(CMD_GAMMASET);
    write_data(0x01);  // Gamma curve 1
    
    // Positive Gamma Correction
    write_command(0xE0);
    write_data(0x0F);
    write_data(0x31);
    write_data(0x2B);
    write_data(0x0C);
    write_data(0x0E);
    write_data(0x08);
    write_data(0x4E);
    write_data(0xF1);
    write_data(0x37);
    write_data(0x07);
    write_data(0x10);
    write_data(0x03);
    write_data(0x0E);
    write_data(0x09);
    write_data(0x00);
    
    // Negative Gamma Correction
    write_command(0xE1);
    write_data(0x00);
    write_data(0x0E);
    write_data(0x14);
    write_data(0x03);
    write_data(0x11);
    write_data(0x07);
    write_data(0x31);
    write_data(0xC1);
    write_data(0x48);
    write_data(0x08);
    write_data(0x0F);
    write_data(0x0C);
    write_data(0x31);
    write_data(0x36);
    write_data(0x0F);
    
    // Exit Sleep
    write_command(CMD_SLPOUT);
    delay_ms(120);
    
    // Display ON
    write_command(CMD_DISPON);
    delay_ms(120);
    
    // Increase SPI speed for normal operation (4MHz)
    // 18.125MHz / 4MHz = 4.5, so div = 3
    SPI_SCKDIV = 3;
}

void ili9341_fill_screen(uint16_t color) {
    // Set column address (0 to 239)
    write_command(CMD_CASET);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0xEF);  // 239
    
    // Set page address (0 to 319)
    write_command(CMD_PASET);
    write_data(0x00);
    write_data(0x00);
    write_data(0x01);
    write_data(0x3F);  // 319
    
    // Memory write
    write_command(CMD_RAMWR);
    
    gpio_set(ILI9341_DC_PIN, 1);  // Data mode
    gpio_set(ILI9341_CS_PIN, 0);  // CS low
    
    // Fill with color
    for (uint32_t i = 0; i < 76800; i++) {  // 240 * 320
        spi_xfer(color >> 8);    // High byte
        spi_xfer(color & 0xFF);  // Low byte
    }
    
    gpio_set(ILI9341_CS_PIN, 1);  // CS high
}

void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= _width || y >= _height) return;
    
    // Set column address
    write_command(CMD_CASET);
    write_data(x >> 8);
    write_data(x & 0xFF);
    write_data(x >> 8);
    write_data(x & 0xFF);
    
    // Set page address
    write_command(CMD_PASET);
    write_data(y >> 8);
    write_data(y & 0xFF);
    write_data(y >> 8);
    write_data(y & 0xFF);
    
    // Write pixel
    write_command(CMD_RAMWR);
    write_data(color >> 8);
    write_data(color & 0xFF);
}
