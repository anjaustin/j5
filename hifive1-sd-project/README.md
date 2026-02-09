# HiFive1 SD Card Test Project

Tests microSD card access on HiFive1 Rev B with Arduino LCD Shield.

## Hardware

- **Board:** SiFive HiFive1 Rev B01 (Original, ~7-8 years old)
- **Shield:** Arduino LCD Shield with microSD slot
- **microSD:** 8GB card in shield slot
- **Connection:** USB to Raspberry Pi 4

## Pin Mapping

### SPI (for SD Card)
- MOSI: GPIO 3 (Arduino D11)
- MISO: GPIO 4 (Arduino D12)
- SCK:  GPIO 5 (Arduino D13)
- CS:   GPIO 10 (Arduino D10)

### UART (for Debug Output)
- TX: GPIO 17
- RX: GPIO 16
- Baud: 115200

## Building

```bash
cd ~/hifive1-sd-project
make
```

This creates:
- `hifive1_sd_test.elf` - ELF executable
- `hifive1_sd_test.hex` - Intel HEX for flashing
- `hifive1_sd_test.lst` - Disassembly listing

## Flashing

### Method 1: Drag and Drop (Easiest)
```bash
make flash-dragdrop
```

Or manually:
```bash
cp hifive1_sd_test.hex /media/ztflynn/HiFive/
```

The board will automatically flash and reset.

### Method 2: OpenOCD
```bash
make flash
```

## Monitoring Output

Connect to serial console:
```bash
screen /dev/ttyACM1 115200
```

Or use minicom:
```bash
minicom -D /dev/ttyACM1 -b 115200
```

## Expected Output

```
========================================
  HiFive1 SD Card Test
  Arduino LCD Shield + microSD
========================================

Initializing SPI...
SPI initialized at 500kHz

SD Card Init Starting...
CMD0 Response: 0x00000001
SD Card in idle state
CMD8 Response: 0x00000001 (SDv2+ card detected)

SD Card initialized successfully!

*** SD Card Ready! ***
You can now read/write blocks

Entering main loop...
Heartbeat: 0000000A seconds
```

## Troubleshooting

### Board not detected
- Check USB cable (must be data cable, not power-only)
- Try USB 2.0 port instead of 3.0
- Check power supply (Pi needs stable 5V, 3A)

### SD card not responding
- Ensure card is properly inserted
- Check card is not write-protected
- Try different SD card
- Shield may draw too much power (use powered USB hub)

### Serial output garbled
- Wrong baud rate (should be 115200)
- Check /dev/ttyACM1 exists
- Add user to dialout group: `sudo usermod -a -G dialout $USER`

## Files

- `hifive1_sd_test.c` - Main program
- `crt0.s` - Startup code
- `hifive1.ld` - Linker script
- `Makefile` - Build system

## Next Steps

To actually read/write files:
1. Implement block read (CMD17)
2. Add FAT filesystem support (FatFs library)
3. Parse MBR and partition table
4. Implement file operations

See: http://elm-chan.org/fsw/ff/00index_e.html (FatFs)
