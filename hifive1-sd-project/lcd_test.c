/**
 * lcd_test.c - Wake up the 2.8" ILI9341 display
 */

#include "ili9341.h"

// Simple delay
void delay(volatile uint32_t count) {
    while(count--);
}

int main(void) {
    // Initialize the display
    ili9341_init();
    
    // Test pattern - cycle through colors
    while(1) {
        ili9341_fill_screen(ILI9341_RED);
        delay(10000000);
        
        ili9341_fill_screen(ILI9341_GREEN);
        delay(10000000);
        
        ili9341_fill_screen(ILI9341_BLUE);
        delay(10000000);
        
        ili9341_fill_screen(ILI9341_WHITE);
        delay(10000000);
        
        ili9341_fill_screen(ILI9341_BLACK);
        delay(10000000);
    }
    
    return 0;
}
