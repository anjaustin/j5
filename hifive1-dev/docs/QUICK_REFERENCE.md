# Quick Reference Guide
## HiFive1 Rev B01 Development

---

## One-Page Cheat Sheet

### Connection
```bash
# Verify board detected
lsusb | grep HiFive          # Should show: 1366:1061
ls /dev/ttyACM*              # Should show: ttyACM0, ttyACM1
```

### Build
```bash
cd hifive1-dev
make                          # Creates .hex file
```

### Flash
```bash
# Method 1: Drag and drop
cp src/risc-v/hifive1_sd_test.hex /media/$USER/HiFive/

# Method 2: OpenOCD
make flash
```

### Debug
```bash
# Terminal 1: OpenOCD
openocd -f board/sifive-hifive1-revb.cfg

# Terminal 2: GDB
riscv64-unknown-elf-gdb src/risc-v/hifive1_sd_test.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

### Monitor
```bash
# ESP32 serial
screen /dev/ttyACM1 115200    # Ctrl+A,K to exit
# or
minicom -D /dev/ttyACM1 -b 115200
```

---

## Memory Quick Reference

| Region | Address | Size | Purpose |
|--------|---------|------|---------|
| Flash | 0x2000_0000 | 4MB | Program storage |
| RAM | 0x8000_0000 | 16KB | Runtime data |
| GPIO | 0x1001_2000 | - | Pin control |
| UART | 0x1001_3000 | - | Serial communication |
| SPI | 0x1001_4000 | - | SPI0 for flash |

---

## GPIO Quick Reference

**Arduino D10-D13 (SPI for SD Card):**
```
D10 (CS)   → GPIO 2
D11 (MOSI) → GPIO 3
D12 (MISO) → GPIO 4
D13 (SCK)  → GPIO 5
```

**Arduino D0-D1 (UART):**
```
D0 (RX) → GPIO 16 (ESP32 default)
D1 (TX) → GPIO 17 (ESP32 default)
```

**Analog Pins:**
```
A0-A3 → GPIO 0-3 (ADC capable)
A4-A5 → GPIO 22-23 (I2C capable)
```

---

## OpenOCD Commands

### Connection
```tcl
# In OpenOCD telnet (port 4444)
telnet localhost 4444

# Or via GDB
riscv64-unknown-elf-gdb
(gdb) target remote localhost:3333
```

### Essential Commands
```tcl
# Reset
reset halt
reset run

# Examine memory
mdw 0x20000000 16      # Read 16 words from flash
mdw 0x80000000 16      # Read 16 words from RAM

# Registers
reg pc                 # Read PC
reg x1                 # Read register x1 (ra)
reg                    # Read all registers

# Flash operations
flash erase_sector 0 0 127    # Erase all 128 sectors
flash write_image file.hex    # Write hex file
flash verify file.hex         # Verify flash contents

# Breakpoints
bp 0x20001000 4 hw     # Hardware breakpoint
rbp 0x20001000         # Remove breakpoint

# Step/Run
step                   # Single step
resume                 # Run
halt                   # Stop
```

### GDB Commands
```gdb
# Load and run
file program.elf       # Load symbols
target remote :3333    # Connect
display/i $pc          # Show disassembly
load                   # Flash program
break main             # Set breakpoint
continue               # Run
stepi                  # Step one instruction
info registers         # Show all registers
x/10xw 0x20000000      # Examine memory
monitor reset halt     # Reset via OpenOCD
quit                   # Exit
```

---

## Code Snippets

### Bare-Minimum Program
```c
// main.c - Bare minimum RISC-V program

void _start(void) {
    // Your code here
    
    // Infinite loop
    while(1);
}
```

### UART Output
```c
// UART0 registers
#define UART_TXFIFO (*(volatile uint32_t *)0x10013000)
#define UART_DIV    (*(volatile uint32_t *)0x10013018)

void uart_init(void) {
    UART_DIV = 139;  // 115200 @ 16MHz
}

void uart_putc(char c) {
    while (UART_TXFIFO & 0x80000000);
    UART_TXFIFO = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}
```

### GPIO Control
```c
// GPIO registers
#define GPIO_OUTPUT_EN (*(volatile uint32_t *)0x10012008)
#define GPIO_PORT      (*(volatile uint32_t *)0x1001200C)

// Set pin as output
GPIO_OUTPUT_EN |= (1 << pin);

// Set pin high
GPIO_PORT |= (1 << pin);

// Set pin low
GPIO_PORT &= ~(1 << pin);

// Toggle pin
GPIO_PORT ^= (1 << pin);
```

### SPI Transfer
```c
// SPI0 registers
#define SPI_TXDATA (*(volatile uint32_t *)0x10014048)
#define SPI_RXDATA (*(volatile uint32_t *)0x1001404C)
#define SPI_SCKDIV (*(volatile uint32_t *)0x10014000)

void spi_init(void) {
    SPI_SCKDIV = 31;  // 500kHz @ 16MHz
}

uint8_t spi_transfer(uint8_t data) {
    while (SPI_TXDATA & 0x80000000);
    SPI_TXDATA = data;
    uint32_t rx;
    do { rx = SPI_RXDATA; } while (rx & 0x80000000);
    return (uint8_t)rx;
}
```

### Delay
```c
void delay_ms(volatile uint32_t ms) {
    // Rough delay at 16MHz
    for (volatile uint32_t i = 0; i < ms * 3200; i++);
}
```

---

## Makefile Template

```makefile
PREFIX = riscv64-unknown-elf
CC = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump

TARGET = my_program

CFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medlow
CFLAGS += -O0 -g -Wall -Wextra
CFLAGS += -nostdlib -nostartfiles -ffreestanding

LDFLAGS = -T hifive1.ld -nostdlib -Wl,--gc-sections

OBJS = $(TARGET).o crt0.o

all: $(TARGET).hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(TARGET).hex
	cp $< /media/$(USER)/HiFive/

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean
```

---

## Linker Script Template

```ld
MEMORY
{
    flash (rx) : ORIGIN = 0x20000000, LENGTH = 4M
    ram (rwx)  : ORIGIN = 0x80000000, LENGTH = 16K
}

SECTIONS
{
    .text : {
        *(.text.init)
        *(.text)
        *(.rodata)
        . = ALIGN(4);
    } > flash

    .data : {
        _data_start = .;
        *(.data)
        . = ALIGN(4);
        _data_end = .;
    } > ram AT > flash

    _data_load = LOADADDR(.data);

    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    _stack_top = 0x80004000;
}

ENTRY(_start)
```

---

## Startup Code Template

```asm
# crt0.s - Startup code

.section .text.init

.global _start
_start:
    # Initialize stack
    la sp, _stack_top

    # Copy .data from flash to RAM
    la t0, _data_load
    la t1, _data_start
    la t2, _data_end
1:
    bgeu t1, t2, 2f
    lw t3, 0(t0)
    sw t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    j 1b
2:

    # Clear BSS
    la t0, _bss_start
    la t1, _bss_end
1:
    bgeu t0, t1, 2f
    sw zero, 0(t0)
    addi t0, t0, 4
    j 1b
2:

    # Call main
    call main

    # Hang if main returns
1:
    j 1b
```

---

## Pin Number Reference

**Digital Pins (Arduino → RISC-V GPIO):**
```
D0  → 16    D7  → 23
D1  → 17    D8  → 0
D2  → 18    D9  → 1
D3  → 19    D10 → 2
D4  → 20    D11 → 3
D5  → 21    D12 → 4
D6  → 22    D13 → 5
```

**Analog Pins:**
```
A0 → 0    A3 → 3
A1 → 1    A4 → 22 (I2C SDA)
A2 → 2    A5 → 23 (I2C SCL)
```

**Special Functions:**
```
UART0: GPIO 16 (RX), 17 (TX)
SPI0:  GPIO 3 (MOSI), 4 (MISO), 5 (SCK), 2 (CS)
I2C:   GPIO 22 (SDA), 23 (SCL)
```

---

## Common Hex Values

**UART Baud Rate Divisors (at 16MHz):**
| Baud | Divisor |
|------|---------|
| 9600 | 1667 |
| 19200 | 833 |
| 38400 | 417 |
| 57600 | 278 |
| 115200 | 139 |

**SPI Clock Divisors (at 16MHz):**
| Divisor | SPI Clock |
|---------|-----------|
| 31 | 500 kHz |
| 15 | 1 MHz |
| 7 | 2 MHz |
| 3 | 4 MHz |
| 1 | 8 MHz |
| 0 | 16 MHz (max) |

---

## Troubleshooting Checklist

□ USB cable is data-capable (not power-only)
□ Board powered with stable 5V (no undervoltage warnings)
□ User in dialout group: `groups $USER | grep dialout`
□ Correct serial device: `/dev/ttyACM1` for ESP32
□ Correct debug interface: JTAG on port 3333 for RISC-V
□ OpenOCD using correct config: `sifive-hifive1-revb.cfg`
□ Shield powered separately if causing disconnects

---

## Useful Commands Summary

```bash
# Check connections
lsusb | grep HiFive
ls /dev/ttyACM*

# Flash
make && cp *.hex /media/$USER/HiFive/

# Debug
openocd -f board/sifive-hifive1-revb.cfg &
riscv64-unknown-elf-gdb program.elf -ex "target remote :3333"

# Monitor
screen /dev/ttyACM1 115200

# Build
make clean && make

# Disassembly
riscv64-unknown-elf-objdump -d program.elf | less

# Symbols
riscv64-unknown-elf-nm program.elf | grep -i main

# Size
riscv64-unknown-elf-size program.elf
```

---

**Print this page and keep it handy!**

Last Updated: February 9, 2026
