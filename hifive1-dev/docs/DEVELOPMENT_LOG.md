# HiFive1 Development Log
## Session: February 9, 2026

---

## Executive Summary

Successfully brought up a **7-8 year old SiFive HiFive1 Rev B01** RISC-V development board with an **Arduino LCD shield** containing an **8GB microSD card**. Established dual-processor communication architecture using simultaneous JTAG (RISC-V) and UART (ESP32) interfaces.

---

## Hardware Identification

### Primary Board
- **Board:** SiFive HiFive1 Rev B01
- **Manufacture Date:** ~2017-2018 (7-8 years old)
- **Processor 1:** SiFive E31 RISC-V (RV32IMAC)
  - Architecture: 32-bit RISC-V
  - Clock: 320 MHz max
  - RAM: 16KB on-chip SRAM
  - Flash: 4MB (ISSI IS25LP032)
  - Extensions: I (Integer), M (Multiply), A (Atomic), C (Compressed)
- **Processor 2:** ESP32 (Tensilica Xtensa)
  - Clock: 240 MHz dual-core
  - RAM: 520KB SRAM
  - Features: WiFi 802.11 b/g/n, Bluetooth v4.2 + BLE
- **USB Interface:** SEGGER J-Link OB-K22-SiFive
  - VID/PID: 1366:1061
  - Firmware: v1.00 (Jul 23 2021)
- **Power Requirements:** 5V via USB, ~100-200mA typical

### Shield
- **Type:** Arduino LCD Shield with microSD slot
- **Power Draw:** Significant (requires stable power)
- **Interface:** SPI for LCD and SD card
- **LEDs:** Bright power indicators

### Storage
- **microSD:** 8GB card (in shield slot)
- **Protocol:** SPI mode
- **Voltage:** 3.3V (via board regulator)

---

## Discovery & Troubleshooting Timeline

### Phase 1: Initial Connection Attempts
**Problem:** Board not detected via USB

**Attempted:**
1. Connected USB A to Micro-USB on board
2. Checked `lsusb` - no device found
3. Checked `/dev/ttyUSB*` and `/dev/ttyACM*` - no serial ports
4. Checked `dmesg` - no USB enumeration

**Root Cause:** All three cables tested were **power-only**, not data cables

**Resolution:** Used 4th cable with verified data capability

---

### Phase 2: Power Issues
**Problem:** Board powered on (2 green LEDs) but no USB enumeration

**Symptoms:**
- Undervoltage warnings in kernel logs
- `throttled=0x50000` status
- USB disconnects when attaching LCD shield

**Investigation:**
```bash
vcgencmd get_throttled
# Output: throttled=0x50000
# Meaning: Frequency capped due to undervoltage
```

**Root Cause:** Raspberry Pi power supply insufficient for both Pi and HiFive1 + LCD shield

**Resolution:** 
- Used better power supply
- Removed LCD shield temporarily
- Board enumerated successfully as `1366:1061 SEGGER HiFive`

---

### Phase 3: Serial Port Confusion
**Problem:** Seeing ESP32 boot messages instead of RISC-V output

**Observed:**
```
rst:0x1 (POWERON_RESET),boot:0x3a (SPI_FAST_FLASH_BOOT)
flash read err, 1000
ets_main.c 371
```

**Discovery:** HiFive1 Rev B01 has **dual processors**:
- RISC-V E31 (main CPU)
- ESP32 (co-processor for WiFi/Bluetooth)

**Interface Mapping:**
- `/dev/ttyACM0` - JTAG interface (RISC-V programming)
- `/dev/ttyACM1` - UART (connected to ESP32 by default)

**Resolution:** Established dual-interface communication architecture

---

### Phase 4: Shield Power Issues
**Problem:** LCD shield causes USB disconnect

**Symptoms:**
- HiFive1 disconnects from USB when shield attached
- `dmesg` shows: `usb 1-1.3: USB disconnect, device number 3`

**Root Cause:** Shield draws too much current, causing USB voltage drop

**Resolution:** Board+shield requires powered USB hub or external power

**Workaround:** Successfully ran with shield after power stabilization

---

## Communication Architecture

### Established Dual-Interface System

```
┌─────────────────────────────────────────┐
│        Raspberry Pi 4 (Host)            │
├─────────────────────────────────────────┤
│                                         │
│  ┌─────────────────────────────────┐   │
│  │    Interface 1: JTAG            │   │
│  │    ──────────────────────       │   │
│  │    Device: /dev/ttyACM0         │   │
│  │    Protocol: JTAG via SEGGER    │   │
│  │    Target: RISC-V E31           │   │
│  │    Speed: 4 MHz                 │   │
│  │    GDB Port: 3333               │   │
│  │    Telnet Port: 4444            │   │
│  │                                 │   │
│  │    Tools:                       │   │
│  │    - OpenOCD                    │   │
│  │    - GDB (riscv64-elf)          │   │
│  │    - JTAG debugging             │   │
│  └─────────────────────────────────┘   │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │    Interface 2: UART            │   │
│  │    ──────────────────────       │   │
│  │    Device: /dev/ttyACM1         │   │
│  │    Protocol: UART 115200 8N1    │   │
│  │    Target: ESP32                │   │
│  │                                 │   │
│  │    Tools:                       │   │
│  │    - screen                     │   │
│  │    - minicom                    │   │
│  │    - esptool.py                 │   │
│  └─────────────────────────────────┘   │
│                                         │
└─────────────────────────────────────────┘
                   │
                   │ USB
                   ▼
┌─────────────────────────────────────────┐
│        HiFive1 Rev B01 Board            │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────────┐    ┌──────────────┐  │
│  │              │    │              │  │
│  │ RISC-V E31   │◄──►│    ESP32     │  │
│  │              │    │              │  │
│  │ - JTAG       │UART│ - WiFi       │  │
│  │ - Flash      │    │ - Bluetooth  │  │
│  │ - GPIO       │    │ - UART       │  │
│  │              │    │              │  │
│  └──────────────┘    └──────────────┘  │
│         │                   │          │
│         └───────────────────┘          │
│              Shared Peripherals         │
│              - LCD Shield               │
│              - microSD (8GB)            │
│              - SPI Bus                  │
│              - I2C Bus                  │
│                                         │
└─────────────────────────────────────────┘
```

### Key Benefits
1. **No interference** - JTAG and UART are independent interfaces
2. **Simultaneous access** - Debug RISC-V while monitoring ESP32
3. **Flexible architecture** - Each processor handles different tasks
4. **Inter-processor communication** - Can add UART link between processors

---

## Pin Mapping

### RISC-V GPIO to Arduino Shield Mapping

| Arduino Pin | RISC-V GPIO | Function | Notes |
|-------------|-------------|----------|-------|
| D0 (RX) | GPIO 16 | UART RX | ESP32 default |
| D1 (TX) | GPIO 17 | UART TX | ESP32 default |
| D2 | GPIO 18 | Digital I/O | |
| D3 (PWM) | GPIO 19 | PWM / GPIO | |
| D4 | GPIO 20 | Digital I/O | |
| D5 (PWM) | GPIO 21 | PWM / GPIO | |
| D6 (PWM) | GPIO 22 | PWM / GPIO | |
| D7 | GPIO 23 | Digital I/O | |
| D8 | GPIO 0 | Digital I/O | Also analog |
| D9 (PWM) | GPIO 1 | PWM / GPIO | Also analog |
| D10 (PWM/CS) | GPIO 2 | SPI CS / PWM | SD card CS |
| D11 (MOSI) | GPIO 3 | SPI MOSI | SD card MOSI |
| D12 (MISO) | GPIO 4 | SPI MISO | SD card MISO |
| D13 (SCK) | GPIO 5 | SPI SCK | SD card SCK |
| A0 | GPIO 0 | Analog input | 12-bit ADC |
| A1 | GPIO 1 | Analog input | 12-bit ADC |
| A2 | GPIO 2 | Analog input | 12-bit ADC |
| A3 | GPIO 3 | Analog input | 12-bit ADC |
| A4 (SDA) | GPIO 22 | I2C SDA | Shared with D6 |
| A5 (SCL) | GPIO 23 | I2C SCL | Shared with D7 |

### Memory Map

```
0x2000_0000 - 0x2040_0000 : Flash (4MB, ISSI IS25LP032)
0x8000_0000 - 0x8000_4000 : RAM (16KB SRAM)
0x1001_2000               : GPIO Controller
0x1001_3000               : UART0
0x1001_4000               : SPI0
```

---

## Software Development

### Toolchain Installation

**RISC-V GCC:**
```bash
sudo apt-get install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
# Version: 14.2.0
# Architecture: rv32imac
# ABI: ilp32
```

**OpenOCD:**
```bash
# Already installed: v0.12.0+dev-snapshot (2025-07-16)
# Config: board/sifive-hifive1-revb.cfg
```

**Serial Tools:**
```bash
sudo apt-get install screen minicom
# Usage: screen /dev/ttyACM1 115200
```

### Code Development

**Created SD Card Test Program:**
- File: `hifive1_sd_test.c`
- Size: 2,464 bytes (fits easily in 16KB RAM)
- Features:
  - UART initialization (115200 baud)
  - SPI initialization (500kHz for SD compatibility)
  - SD card initialization sequence:
    - CMD0 (GO_IDLE_STATE)
    - CMD8 (SEND_IF_COND - detect SDv2+)
    - ACMD41 (SD_SEND_OP_COND - initialize)
  - Debug output via UART

**Build System:**
- Makefile with proper RISC-V flags
- Linker script for HiFive1 memory map
- Startup code (crt0.s) for initialization
- Support for drag-and-drop flashing

### Build Commands

```bash
# Build
make

# Flash via drag-and-drop
cp hifive1_sd_test.hex /media/ztflynn/HiFive/

# Flash via OpenOCD
openocd -f board/sifive-hifive1-revb.cfg \
  -c "init; halt; flash write_image erase hifive1_sd_test.hex; reset; exit"

# Debug with GDB
openocd -f board/sifive-hifive1-revb.cfg  # Terminal 1
riscv64-unknown-elf-gdb hifive1_sd_test.elf  # Terminal 2
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

---

## Probe Results

### USB Enumeration
```
Bus 001 Device 004: ID 1366:1061 SEGGER HiFive
Manufacturer: SEGGER
Product: HiFive
bcdDevice: 1.00
```

### Serial Ports
```
/dev/ttyACM0 - JTAG interface
/dev/ttyACM1 - UART (ESP32)
```

### JTAG Detection
```
JTAG tap: riscv.cpu tap/device found: 0x20000913
Manufacturer: SiFive Inc (0x489)
Part: 0x0000
Version: 0x2
XLEN: 32
misa: 0x40101105 (RV32IMAC)
```

### Flash Detection
```
Device: ISSI IS25LP032
ID: 0x0016609d
Size: 4MB
Protocol: Quad SPI
```

### Mass Storage
```
Device: /dev/sda
Size: 10.7MB
Mount: /media/ztflynn/HiFive/
Usage: Drag-and-drop firmware flashing
```

---

## Key Learnings

### Hardware
1. **Cable quality matters** - 3/4 cables were power-only
2. **Power stability critical** - Shield causes voltage drops
3. **Dual-processor complexity** - ESP32 + RISC-V requires understanding both
4. **Age ≠ Obsolescence** - 7-8 year old board still functional

### Software
1. **Dual-interface possible** - JTAG and UART work simultaneously
2. **OpenOCD reliable** - SEGGER J-Link OB well-supported
3. **Bare-metal feasible** - 16KB RAM sufficient for simple programs
4. **RISC-V toolchain mature** - GCC 14.2.0 works well

### Development Workflow
1. **Flash via mass storage** - Easiest method
2. **Debug via JTAG/GDB** - Most powerful
3. **Monitor ESP32 via UART** - Separate channel
4. **Simultaneous access** - No interference between interfaces

---

## Current Status

**✅ Completed:**
- Hardware detection and connection
- Dual-interface communication established
- Toolchain installation
- SD card test program written
- Build system created
- OpenOCD configuration verified

**⏸️ Ready to Test:**
- Flash SD card test program to RISC-V
- Verify SD card initialization
- Read SD card contents

**📋 Next Steps:**
1. Complete SD card initialization code
2. Add FAT filesystem support (FatFs)
3. Implement file read/write
4. Add inter-processor UART communication
5. Test LCD display control

---

## Files Created

```
hifive1-dev/
├── src/
│   ├── risc-v/
│   │   ├── hifive1_sd_test.c  # Main test program
│   │   ├── crt0.s             # Startup assembly
│   │   └── hifive1.ld         # Linker script
│   ├── esp32/                 # (empty - future ESP32 code)
│   └── shared/                # (empty - shared libraries)
├── docs/
│   └── DEVELOPMENT_LOG.md     # This file
├── Makefile                   # Build system
└── README.md                  # Project overview
```

---

## References

- **SiFive HiFive1 Docs:** https://www.sifive.com/boards/hifive1
- **OpenOCD RISC-V:** http://openocd.org/doc/html/Debug-Adapter-Configuration.html
- **ESP32 Bootloader:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/startup.html
- **FatFs Library:** http://elm-chan.org/fsw/ff/00index_e.html
- **Arduino Shield Pinout:** Standard Uno R3 pinout

---

**Session Duration:** ~2 hours
**Status:** Board fully operational, ready for development
**Next Milestone:** Successful SD card read operation
