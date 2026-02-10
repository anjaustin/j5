/**
 * gpio_toggle_test.c - Test GPIO pin toggling for debugging
 * 
 * Toggles CS, DC, and RST pins slowly so you can measure with multimeter/LED
 */

#include <stdint.h>

#define GPIO_BASE       0x10012000
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))

// Pin definitions
#define PIN_CS          2   // D10
#define PIN_DC          1   // D9
#define PIN_RST         0   // D8

void delay(volatile uint32_t count) {
    while(count--);
}

int main(void) {
    // Configure pins as outputs
    GPIO_OUTPUT_EN |= (1 << PIN_CS) | (1 << PIN_DC) | (1 << PIN_RST);
    
    // Set all high initially
    GPIO_PORT |= (1 << PIN_CS) | (1 << PIN_DC) | (1 << PIN_RST);
    
    while(1) {
        // Toggle CS (D10) - should see 3.3V/0V swinging
        GPIO_PORT ^= (1 << PIN_CS);
        delay(5000000);  // ~1 second
        
        // Toggle DC (D9)
        GPIO_PORT ^= (1 << PIN_DC);
        delay(5000000);
        
        // Toggle RST (D8)
        GPIO_PORT ^= (1 << PIN_RST);
        delay(5000000);
    }
    
    return 0;
}
