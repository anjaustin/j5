/**
 * diskio.c - FatFs Disk I/O layer for HiFive1 SD card
 * 
 * Connects FatFs to our SPI SD card driver
 */

#include <stdint.h>
#include "source/ff.h"         // FatFs definitions (must come first!)
#include "source/diskio.h"     // FatFs disk I/O layer

#ifndef NULL
#define NULL ((void *)0)
#endif

// Additional SD card commands
#define CMD12   12
#define CMD18   18
#define CMD25   25

// Card type flags (from ff.c)
#define CT_MMC      0x01
#define CT_SD1      0x02
#define CT_SD2      0x04
#define CT_SDC      (CT_SD1|CT_SD2)
#define CT_BLOCK    0x08

// Our SD card SPI interface (from hifive1_probe.c)
#define GPIO_BASE       0x10012000
#define SPI1_BASE       0x10024000

#define GPIO_IOF_EN     (*(volatile uint32_t *)(GPIO_BASE + 0x38))
#define GPIO_IOF_SEL    (*(volatile uint32_t *)(GPIO_BASE + 0x3C))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_PORT       (*(volatile uint32_t *)(GPIO_BASE + 0x0C))

#define SPI_SCKDIV      (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI_SCKMODE     (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI_FMT         (*(volatile uint32_t *)(SPI1_BASE + 0x40))
#define SPI_TXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x48))
#define SPI_RXDATA      (*(volatile uint32_t *)(SPI1_BASE + 0x4C))

#define PIN_SPI1_MOSI   3
#define PIN_SPI1_MISO   4
#define PIN_SPI1_SCK    5
#define PIN_SD_CS       2

// SD Card commands
#define CMD0    0
#define CMD1    1
#define CMD8    8
#define CMD16   16
#define CMD17   17
#define CMD24   24
#define CMD55   55
#define CMD58   58
#define ACMD41  41

static uint8_t CardType = 0;  // Card type flag

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

static void cs_low(void) {
    GPIO_PORT &= ~(1 << PIN_SD_CS);
}

static void cs_high(void) {
    GPIO_PORT |= (1 << PIN_SD_CS);
}

static void spi_cs(uint8_t high) {
    if (high) cs_high();
    else cs_low();
}

// Send SD command
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t n, res;
    
    if (cmd & 0x80) {  // ACMD<n> is the command sequence of CMD55-CMD<n>
        cmd &= 0x7F;
        res = sd_send_cmd(CMD55, 0);
        if (res > 1) return res;
    }
    
    // Select the card
    spi_cs(1);
    spi_xfer(0xFF);
    spi_cs(0);
    spi_xfer(0xFF);
    
    // Send command packet
    spi_xfer(0x40 | cmd);
    spi_xfer((uint8_t)(arg >> 24));
    spi_xfer((uint8_t)(arg >> 16));
    spi_xfer((uint8_t)(arg >> 8));
    spi_xfer((uint8_t)arg);
    
    // Send CRC
    uint8_t crc = 0xFF;
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    spi_xfer(crc);
    
    // Wait for response
    for (n = 10; n; n--) {
        res = spi_xfer(0xFF);
        if (!(res & 0x80)) break;
    }
    
    return res;
}

// Initialize SD card
static int sd_init(void) {
    uint8_t n, cmd, ty, ocr[4];
    uint16_t tmr;
    
    // Setup SPI
    GPIO_IOF_EN |= (1 << PIN_SPI1_MOSI) | (1 << PIN_SPI1_MISO) | (1 << PIN_SPI1_SCK);
    GPIO_IOF_SEL &= ~((1 << PIN_SPI1_MOSI) | (1 << PIN_SPI1_MISO) | (1 << PIN_SPI1_SCK));
    
    GPIO_OUTPUT_EN |= (1 << PIN_SD_CS);
    cs_high();
    
    SPI_SCKDIV = 100;  // Slow speed for init
    SPI_SCKMODE = 0;
    SPI_FMT = 0x00080000;
    
    // Send 80 dummy clocks
    for (n = 20; n; n--) spi_xfer(0xFF);
    
    ty = 0;
    if (sd_send_cmd(CMD0, 0) == 1) {  // Enter Idle state
        if (sd_send_cmd(CMD8, 0x1AA) == 1) {  // SDv2
            for (n = 0; n < 4; n++) ocr[n] = spi_xfer(0xFF);
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                for (tmr = 1000; tmr; tmr--) {
                    if (sd_send_cmd(ACMD41, 0x40000000) == 0) break;
                }
                if (tmr && sd_send_cmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) ocr[n] = spi_xfer(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  // SDv2 (BLOCK or BYTE)
                }
            }
        } else {  // SDv1 or MMCv3
            if (sd_send_cmd(ACMD41, 0) <= 1) {
                ty = CT_SD1;
                cmd = ACMD41;
            } else {
                ty = CT_MMC;
                cmd = CMD1;
            }
            for (tmr = 1000; tmr; tmr--) {
                if (sd_send_cmd(cmd, 0) == 0) break;
            }
            if (!tmr || sd_send_cmd(CMD16, 512) != 0) ty = 0;  // Set R/W block length
        }
    }
    
    CardType = ty;
    spi_cs(1);
    spi_xfer(0xFF);
    
    // Increase speed for normal operation
    SPI_SCKDIV = 3;  // ~4.5MHz at 18.125MHz core
    
    return ty ? 0 : STA_NOINIT;
}

// Read sector
static int sd_read(uint8_t *buff, uint32_t sector, uint32_t count) {
    uint8_t cmd;
    
    if (!(CardType & CT_BLOCK)) sector *= 512;  // Convert to byte address
    
    cmd = (count == 1) ? CMD17 : CMD18;  // CMD17: Single block, CMD18: Multi-block
    if (sd_send_cmd(cmd, sector) != 0) return 0;
    
    do {
        uint8_t token;
        uint16_t tmr;
        
        // Wait for data token
        for (tmr = 3000; tmr; tmr--) {
            token = spi_xfer(0xFF);
            if (token != 0xFF) break;
        }
        if (token != 0xFE) return 0;  // Error
        
        // Read data
        for (uint16_t i = 0; i < 512; i++) buff[i] = spi_xfer(0xFF);
        
        // Skip CRC
        spi_xfer(0xFF);
        spi_xfer(0xFF);
        
        buff += 512;
    } while (--count);
    
    if (cmd == CMD18) sd_send_cmd(CMD12, 0);  // Stop multi-block read
    spi_cs(1);
    spi_xfer(0xFF);
    
    return 1;
}

// Write sector
static int sd_write(const uint8_t *buff, uint32_t sector, uint32_t count) {
    uint8_t cmd;
    
    if (!(CardType & CT_BLOCK)) sector *= 512;
    
    cmd = (count == 1) ? CMD24 : CMD25;  // CMD24: Single block, CMD25: Multi-block
    if (sd_send_cmd(cmd, sector) != 0) return 0;
    
    do {
        // Send data token
        spi_xfer((cmd == CMD24) ? 0xFE : 0xFC);
        
        // Send data
        for (uint16_t i = 0; i < 512; i++) spi_xfer(buff[i]);
        
        // Send CRC
        spi_xfer(0xFF);
        spi_xfer(0xFF);
        
        // Check response
        uint8_t resp = spi_xfer(0xFF);
        if ((resp & 0x1F) != 0x05) return 0;  // Error
        
        // Wait for busy
        while (spi_xfer(0xFF) == 0);
        
        buff += 512;
    } while (--count);
    
    if (cmd == CMD25) {
        spi_xfer(0xFD);  // Stop token
        while (spi_xfer(0xFF) == 0);
    }
    
    spi_cs(1);
    spi_xfer(0xFF);
    
    return 1;
}

// FatFs diskio interface
DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return (CardType == 0) ? STA_NOINIT : 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return sd_init();
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0) return RES_NOTRDY;
    return sd_read(buff, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0) return RES_NOTRDY;
    return sd_write(buff, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0) return RES_NOTRDY;
    
    switch (cmd) {
        case CTRL_SYNC:
            spi_cs(1);
            spi_xfer(0xFF);
            return RES_OK;
            
        case GET_SECTOR_COUNT:
            *(LBA_t *)buff = 0;  // Not implemented yet
            return RES_OK;
            
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;
            
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;
            
        default:
            return RES_PARERR;
    }
}
