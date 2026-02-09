# HiFive1 Dual-Processor Development Platform

**A modern development environment for the SiFive HiFive1 Rev B01 RISC-V board with ESP32 co-processor.**

[![RISC-V](https://img.shields.io/badge/RISC--V-RV32IMAC-blue)](https://riscv.org/)
[![ESP32](https://img.shields.io/badge/ESP32-WiFi%2FBLE-green)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

---

## 🎯 Overview

This project provides a complete development environment for the **SiFive HiFive1 Rev B01** board - a dual-processor development platform featuring:

- **RISC-V E31** (320 MHz, 16KB RAM) - Real-time coordination and control
- **ESP32** (240 MHz dual-core, 520KB RAM) - WiFi, Bluetooth, and high-level processing
- **Arduino LCD Shield** - Display and 8GB microSD storage
- **Dual-interface communication** - Simultaneous JTAG (RISC-V) and UART (ESP32) access

## 🏗️ Architecture

```
┌─────────────────────────────────────────┐
│         Development Host (Pi/PC)         │
│  ┌─────────────────────────────────┐   │
│  │  JTAG → RISC-V (Debug/Program)  │   │
│  │  Port: localhost:3333 (GDB)     │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │  UART → ESP32 (Console/Flash)   │   │
│  │  Device: /dev/ttyACM1           │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
                   │
                   │ USB 1366:1061 (SEGGER)
                   ▼
┌─────────────────────────────────────────┐
│      HiFive1 Rev B01 Board              │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────────┐    ┌──────────────┐  │
│  │ RISC-V E31   │◄──►│ ESP32        │  │
│  │ 320 MHz      │UART│ 240 MHz      │  │
│  │ 16KB RAM     │    │ 520KB RAM    │  │
│  │ 4MB Flash    │    │ WiFi/BT      │  │
│  │ Real-time    │    │ Networking   │  │
│  └──────────────┘    └──────────────┘  │
│         │                   │          │
│         └───────────────────┘          │
│         Shared Peripherals:            │
│         • Arduino LCD Shield           │
│         • 8GB microSD (SPI)            │
│         • GPIO, I2C, SPI, UART         │
│                                         │
└─────────────────────────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

```bash
# Install RISC-V toolchain
sudo apt-get install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf

# Install OpenOCD (for JTAG debugging)
sudo apt-get install openocd

# Install serial communication tools
sudo apt-get install screen minicom

# Add user to dialout group (for serial access)
sudo usermod -a -G dialout $USER
# Log out and back in for group change to take effect
```

### Build

```bash
cd hifive1-dev
make
```

### Flash

**Option 1: Drag and Drop (Easiest)**
```bash
make flash-dragdrop
# Or manually:
cp hifive1_sd_test.hex /media/$USER/HiFive/
```

**Option 2: OpenOCD**
```bash
make flash
```

### Debug

**Terminal 1 - Start OpenOCD:**
```bash
openocd -f board/sifive-hifive1-revb.cfg
```

**Terminal 2 - Connect GDB:**
```bash
riscv64-unknown-elf-gdb src/risc-v/hifive1_sd_test.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) continue
```

**Terminal 3 - Monitor ESP32:**
```bash
screen /dev/ttyACM1 115200
# Press Ctrl+A, K to exit
```

## 📁 Project Structure

```
hifive1-dev/
├── README.md                 # This file
├── Makefile                  # Build system
├── docs/
│   ├── DEVELOPMENT_LOG.md    # Complete session history
│   ├── ARCHITECTURE.md       # Hardware details
│   └── TROUBLESHOOTING.md    # Common issues
├── src/
│   ├── risc-v/              # RISC-V code
│   │   ├── hifive1_sd_test.c
│   │   ├── crt0.s           # Startup code
│   │   └── hifive1.ld       # Linker script
│   ├── esp32/               # ESP32 code (future)
│   └── shared/              # Shared libraries
├── tools/                   # Development utilities
├── scripts/                 # Automation scripts
├── examples/                # Example projects
└── tests/                   # Test suites
```

## 🔧 Hardware Details

### Board Specifications

| Feature | RISC-V E31 | ESP32 |
|---------|-----------|-------|
| **Architecture** | RV32IMAC | Xtensa LX6 |
| **Clock** | 320 MHz | 240 MHz (dual-core) |
| **RAM** | 16KB SRAM | 520KB SRAM |
| **Flash** | 4MB (ISSI IS25LP032) | Shared with RISC-V |
| **Extensions** | I, M, A, C | - |
| **Connectivity** | JTAG | WiFi 802.11b/g/n, Bluetooth 4.2 |
| **USB** | SEGGER J-Link OB | Via shared USB |

### Pin Mapping

**Arduino Shield to RISC-V GPIO:**

| Arduino | GPIO | Function |
|---------|------|----------|
| D0/D1 | 16/17 | UART (ESP32 default) |
| D10 | 2 | SD Card CS |
| D11 | 3 | SD Card MOSI |
| D12 | 4 | SD Card MISO |
| D13 | 5 | SD Card SCK |
| A0-A3 | 0-3 | ADC (12-bit) |
| A4/A5 | 22/23 | I2C SDA/SCL |

### Memory Map

```
0x2000_0000 - 0x2040_0000 : Flash (4MB)
0x8000_0000 - 0x8000_4000 : RAM (16KB)
0x1001_2000              : GPIO
0x1001_3000              : UART0
0x1001_4000              : SPI0
```

## 📝 Development Workflow

### 1. Write Code
Edit `src/risc-v/*.c` or create new files

### 2. Build
```bash
make
```

### 3. Flash
```bash
make flash-dragdrop
```

### 4. Debug
```bash
# Terminal 1
openocd -f board/sifive-hifive1-revb.cfg

# Terminal 2
riscv64-unknown-elf-gdb src/risc-v/hifive1_sd_test.elf
(gdb) target remote :3333
(gdb) load
(gdb) continue
```

### 5. Monitor
```bash
screen /dev/ttyACM1 115200
```

## 🐛 Troubleshooting

### Board not detected
- **Issue:** Power-only USB cable
- **Fix:** Use a data-capable USB cable (test with phone first)

### Undervoltage warnings
- **Issue:** Pi power supply insufficient
- **Fix:** Use official Pi power supply (5.1V, 3A) or powered USB hub

### Shield causes disconnect
- **Issue:** LCD shield draws too much power
- **Fix:** Use powered USB hub or power shield separately

### Serial output garbled
- **Issue:** ESP32 boot messages on UART
- **Note:** Normal - ESP32 boots first. Use JTAG for RISC-V debugging

### Permission denied on /dev/ttyACM*
- **Fix:** `sudo usermod -a -G dialout $USER` then logout/login

See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for more.

## 📊 Current Status

**✅ Working:**
- USB detection and enumeration
- JTAG debugging via OpenOCD
- Dual-interface communication (JTAG + UART)
- Toolchain installation
- Build system
- Basic SD card initialization code

**🚧 In Progress:**
- SD card read/write operations
- FAT filesystem support
- LCD display control

**📋 Planned:**
- ESP32 WiFi integration
- Inter-processor communication
- Sensor integration
- Real-time coordination demos

## 🤝 Contributing

Contributions welcome! Areas needing help:
- FAT filesystem implementation (FatFs port)
- LCD graphics library
- ESP32 networking code
- Documentation improvements
- Example projects

## 📄 License

MIT License - See LICENSE file

## 🙏 Acknowledgments

- SiFive for the HiFive1 board
- SEGGER for J-Link OB
- Espressif for ESP32
- OpenOCD community

## 📞 Support

- **Issues:** Check [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- **Documentation:** See [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md) for complete session history
- **Architecture:** See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for hardware details

---

**Happy Hacking! 🚀**

*Last Updated: February 9, 2026*
