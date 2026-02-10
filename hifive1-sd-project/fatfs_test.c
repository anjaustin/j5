/**
 * fatfs_test.c - Test FatFs on SD card
 */

#include "source/ff.h"  // FatFs header
#include <stdint.h>
#include <stddef.h>

// UART for debug output (simplified)
#define UART0_BASE      0x10013000
#define UART_TXDATA     (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_DIV        (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define GPIO_IOF_EN     (*(volatile uint32_t *)(0x10012000 + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(0x10012000 + 0x3C))

void uart_init(void) {
    GPIO_IOF_EN |= (1 << 16) | (1 << 17);
    GPIO_IOF_SEL &= ~((1 << 16) | (1 << 17));
    UART_DIV = 157;
    UART_TXCTRL = 1;
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

int main(void) {
    uart_init();
    
    uart_puts("\r\n========================================\r\n");
    uart_puts("  FatFs SD Card Test\r\n");
    uart_puts("========================================\r\n\r\n");
    
    FATFS fs;
    FRESULT res;
    
    // Mount the filesystem
    uart_puts("Mounting SD card...\r\n");
    res = f_mount(&fs, "", 1);
    
    if (res != FR_OK) {
        uart_puts("Mount failed! Error: ");
        uart_puthex(res);
        uart_puts("\r\n");
        
        if (res == FR_NO_FILESYSTEM) {
            uart_puts("No filesystem found. Try formatting...\r\n");
        }
    } else {
        uart_puts("SD card mounted successfully!\r\n\r\n");
        
        // Get free space
        DWORD fre_clust;
        FATFS *pfs;
        res = f_getfree("", &fre_clust, &pfs);
        if (res == FR_OK) {
            uint32_t total = (pfs->n_fatent - 2) * pfs->csize / 2;
            uint32_t free = fre_clust * pfs->csize / 2;
            
            uart_puts("Card Info:\r\n");
            uart_puts("  Total: ");
            uart_puthex(total);
            uart_puts(" KB\r\n");
            uart_puts("  Free:  ");
            uart_puthex(free);
            uart_puts(" KB\r\n\r\n");
        }
        
        // Create a test file
        uart_puts("Creating test file...\r\n");
        FIL file;
        res = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
        
        if (res == FR_OK) {
            const char *text = "Hello from HiFive1!\r\nJohnny 5 is alive!\r\n";
            UINT written;
            f_write(&file, text, 44, &written);
            f_close(&file);
            
            uart_puts("Test file created!\r\n");
            uart_puts("Wrote ");
            uart_puthex(written);
            uart_puts(" bytes\r\n\r\n");
            
            // Read it back
            uart_puts("Reading file back...\r\n");
            res = f_open(&file, "test.txt", FA_READ);
            if (res == FR_OK) {
                char buffer[64];
                UINT read;
                f_read(&file, buffer, 63, &read);
                buffer[read] = '\0';
                f_close(&file);
                
                uart_puts("File contents:\r\n");
                uart_puts(buffer);
                uart_puts("\r\n");
            }
        } else {
            uart_puts("Failed to create file! Error: ");
            uart_puthex(res);
            uart_puts("\r\n");
        }
        
        // Unmount
        f_mount(NULL, "", 0);
        uart_puts("\r\nSD card unmounted.\r\n");
    }
    
    uart_puts("\r\nTest complete!\r\n");
    
    while(1);
    return 0;
}
