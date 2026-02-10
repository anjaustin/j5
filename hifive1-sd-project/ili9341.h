/**
 * ili9341.h - ILI9341 TFT LCD Driver for HiFive1 Rev B
 * 
 * 240x320 color TFT display driver using SPI
 * Compatible with Arduino LCD shields
 */

#ifndef ILI9341_H
#define ILI9341_H

#include <stdint.h>

// Pin definitions - TRY DIFFERENT COMBINATIONS
// Arduino D8=GPIO0, D9=GPIO1, D10=GPIO2
// Common configurations:
// Config 1: CS=D10, DC=D9, RST=D8 (2,1,0)
// Config 2: CS=D10, DC=D8, RST=D9 (2,0,1) - SWAPPED
// Config 3: CS=D9, DC=D8, RST=D10 (1,0,2) - DIFFERENT

#define ILI9341_CS_PIN      2   // Back to D10
#define ILI9341_DC_PIN      1   // Back to D9
#define ILI9341_RST_PIN     0   // Back to D8
#define ILI9341_BL_PIN      3   // Try D11 (MOSI) or other pin for backlight?
#define ILI9341_MOSI_PIN    3   // D11 (SPI1)
#define ILI9341_SCK_PIN     5   // D13 (SPI1)

// Display dimensions
#define ILI9341_WIDTH       240
#define ILI9341_HEIGHT      320

// Colors (16-bit RGB565)
#define ILI9341_BLACK       0x0000
#define ILI9341_WHITE       0xFFFF
#define ILI9341_RED         0xF800
#define ILI9341_GREEN       0x07E0
#define ILI9341_BLUE        0x001F
#define ILI9341_YELLOW      0xFFE0
#define ILI9341_CYAN        0x07FF
#define ILI9341_MAGENTA     0xF81F

// Initialize the display
void ili9341_init(void);

// Fill entire screen with color
void ili9341_fill_screen(uint16_t color);

// Draw a pixel
void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

#endif
