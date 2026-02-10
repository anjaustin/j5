# Hardware Architecture Guide
## SiFive HiFive1 Rev B01

---

## Overview

The HiFive1 Rev B01 is a **dual-processor** development board combining a **SiFive E31 RISC-V** processor with an **Espressif ESP32** co-processor on a single board.

---

## Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    HiFive1 Rev B01 Board                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   SiFive E31 (RISC-V)                    │   │
│  │                                                          │   │
│  │  • RV32IMAC Architecture                                 │   │
│  │  • 3-stage in-order pipeline                             │   │
│  │  • CoreMark/MHz: ~2.73                                   │   │
│  │  • DMIPS/MHz: ~1.61                                      │   │
│  │                                                          │   │
│  │  Core Features:                                          │   │
│  │  ├─ RV32I: Base 32-bit integer ISA                       │   │
│  │  ├─ M: Integer multiply/divide                           │   │
│  │  ├─ A: Atomic memory operations                          │   │
│  │  └─ C: Compressed instructions (16-bit)                  │   │
│  │                                                          │   │
│  │  Memory:                                                 │   │
│  │  ├─ 16KB on-chip SRAM @ 0x8000_0000                     │   │
│  │  └─ 4MB QSPI Flash (ISSI IS25LP032) @ 0x2000_0000       │   │
│  │                                                          │   │
│  │  Peripherals:                                            │   │
│  │  ├─ UART x2                                              │   │
│  │  ├─ SPI x2 (one dedicated to flash)                     │   │
│  │  ├─ I2C x1                                               │   │
│  │  ├─ PWM x8                                               │   │
│  │  ├─ GPIO x32                                             │   │
│  │  └─ Watchdog timers x2                                   │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              │ Shared Bus                        │
│                              │ (Inter-processor comms)           │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   ESP32 (Tensilica Xtensa)               │   │
│  │                                                          │   │
│  │  • Dual-core Xtensa LX6 @ 240 MHz                       │   │
│  │  • Ultra-low-power co-processor                          │   │
│  │  • CoreMark: ~1000+ (both cores)                         │   │
│  │                                                          │   │
│  │  Memory:                                                 │   │
│  │  ├─ 520KB SRAM                                           │   │
│  │  └─ 448KB ROM (bootloader)                               │   │
│  │                                                          │   │
│  │  Wireless:                                               │   │
│  │  ├─ 802.11 b/g/n WiFi                                   │   │
│  │  ├─ Bluetooth v4.2 + BLE                                │   │
│  │  └─ On-chip antenna or external U.FL                    │   │
│  │                                                          │   │
│  │  Peripherals:                                            │   │
│  │  ├─ UART x3                                              │   │
│  │  ├─ SPI x4                                               │   │
│  │  ├─ I2C x2                                               │   │
│  │  ├─ PWM x16                                              │   │
│  │  ├─ GPIO x34                                             │   │
│  │  ├─ ADC x18 (12-bit)                                    │   │
│  │  └─ DAC x2 (8-bit)                                      │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              │ Shared Peripherals               │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │               Shared External Interfaces                 │   │
│  │                                                          │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │   │
│  │  │  Arduino     │  │   Arduino    │  │   Arduino    │  │   │
│  │  │   Headers    │  │   Headers    │  │   Headers    │  │   │
│  │  │   (Digital)  │  │   (Analog)   │  │   (Power)    │  │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │   │
│  │                                                          │   │
│  │  Connected Devices:                                      │   │
│  │  ├─ LCD Shield (Arduino compatible)                     │   │
│  │  ├─ 8GB microSD card (SPI interface)                    │   │
│  │  └─ (Other Arduino shields)                             │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              │ USB Interface                     │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              SEGGER J-Link OB (USB Interface)            │   │
│  │                                                          │   │
│  │  • JTAG for RISC-V debugging/programming                │   │
│  │  • UART bridge (ESP32 console)                          │   │
│  │  • USB Mass Storage (drag-and-drop flashing)            │   │
│  │  • VID/PID: 1366:1061                                   │   │
│  │                                                          │   │
│  │  Interfaces exposed:                                     │   │
│  │  ├─ /dev/ttyACM0 - JTAG (RISC-V)                        │   │
│  │  ├─ /dev/ttyACM1 - UART (ESP32)                         │   │
│  │  └─ /dev/sda - Mass storage (10.7MB)                    │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Memory Architecture

### Address Map

```
┌─────────────────────────────────────────────────────────────────┐
│                        Memory Map                               │
├───────────────────┬───────────────────┬─────────────────────────┤
│     Region        │     Address       │        Size             │
├───────────────────┼───────────────────┼─────────────────────────┤
│ Mask ROM          │ 0x0000_1000       │ 8KB                     │
│                   │ (Bootloader)      │                         │
├───────────────────┼───────────────────┼─────────────────────────┤
│ Flash (QSPI)      │ 0x2000_0000       │ 4MB                     │
│                   │ (ISSI IS25LP032)  │ Program storage         │
├───────────────────┼───────────────────┼─────────────────────────┤
│ RAM (SRAM)        │ 0x8000_0000       │ 16KB                    │
│                   │                   │ Runtime data            │
├───────────────────┼───────────────────┼─────────────────────────┤
│ Peripherals       │ 0x1000_0000       │ 256MB region            │
│                   │                   │ Memory-mapped I/O       │
└───────────────────┴───────────────────┴─────────────────────────┘
```

### Peripheral Memory Map

```
0x1000_0000 - 0x1000_0FFF : Reserved
0x1000_1000 - 0x1000_1FFF : CLINT (Core Local Interruptor)
0x1000_2000 - 0x1000_2FFF : PLIC (Platform-Level Interrupt Controller)
0x1001_0000 - 0x1001_0FFF : AON (Always-On domain)
0x1001_1000 - 0x1001_1FFF : PRCI (Power/Reset/Clock/Interrupt)
0x1001_2000 - 0x1001_2FFF : GPIO (General Purpose I/O)
0x1001_3000 - 0x1001_3FFF : UART0
0x1001_4000 - 0x1001_4FFF : SPI0 (Flash)
0x1001_5000 - 0x1001_5FFF : PWM
0x1001_6000 - 0x1001_6FFF : I2C
```

---

## Communication Interfaces

### Dual-Interface Architecture

The HiFive1 provides **two independent communication paths** to the host:

#### Interface 1: JTAG → RISC-V

```
Host (OpenOCD/GDB) ←──JTAG──→ SEGGER J-Link OB ←──JTAG──→ RISC-V E31
```

**Capabilities:**
- Full debugging (breakpoints, single-step, memory inspection)
- Flash programming
- Real-time memory access
- Core register access

**Software Interface:**
- OpenOCD (localhost:4444 telnet, localhost:3333 GDB)
- GDB (riscv64-unknown-elf-gdb)

#### Interface 2: UART → ESP32

```
Host (screen/minicom) ←──UART──→ SEGGER J-Link OB ←──UART──→ ESP32
```

**Capabilities:**
- Serial console
- Firmware upload (esptool.py)
- Debug output
- AT commands

**Software Interface:**
- screen /dev/ttyACM1 115200
- minicom -D /dev/ttyACM1
- esptool.py --port /dev/ttyACM1

### Inter-Processor Communication

The RISC-V and ESP32 can communicate with each other via:

**Option 1: Shared UART**
- Connect RISC-V UART1 to ESP32 UART
- Requires physical wire/jumper

**Option 2: Shared SPI**
- Both processors on same SPI bus
- Master/slave configuration

**Option 3: Shared I2C**
- Both processors on I2C bus
- One as master, one as slave

**Option 4: GPIO Signaling**
- Direct GPIO-to-GPIO connections
- Fast signaling for simple coordination

---

## Pinout Details

### Arduino Shield Headers

#### Digital Pins (D0-D13)

| Shield Pin | RISC-V GPIO | Function | Notes |
|------------|-------------|----------|-------|
| D0 | GPIO 16 | UART RX | Connected to ESP32 by default |
| D1 | GPIO 17 | UART TX | Connected to ESP32 by default |
| D2 | GPIO 18 | GPIO | |
| D3 | GPIO 19 | PWM / GPIO | PWM channel 0 |
| D4 | GPIO 20 | GPIO | |
| D5 | GPIO 21 | PWM / GPIO | PWM channel 1 |
| D6 | GPIO 22 | PWM / GPIO / I2C SDA | PWM channel 2 |
| D7 | GPIO 23 | GPIO / I2C SCL | |
| D8 | GPIO 0 | GPIO / ADC | ADC channel 0 |
| D9 | GPIO 1 | PWM / GPIO / ADC | PWM channel 3, ADC ch 1 |
| D10 | GPIO 2 | PWM / GPIO / ADC / CS | PWM ch 4, ADC ch 2, SPI CS |
| D11 | GPIO 3 | SPI MOSI / ADC | MOSI for SD card, ADC ch 3 |
| D12 | GPIO 4 | SPI MISO | MISO for SD card |
| D13 | GPIO 5 | SPI SCK | Clock for SD card |

#### Analog Pins (A0-A5)

| Shield Pin | RISC-V GPIO | Function | ADC Channel |
|------------|-------------|----------|-------------|
| A0 | GPIO 0 | Analog input | ADC 0 |
| A1 | GPIO 1 | Analog input | ADC 1 |
| A2 | GPIO 2 | Analog input | ADC 2 |
| A3 | GPIO 3 | Analog input | ADC 3 |
| A4 | GPIO 22 | I2C SDA / Analog | I2C or ADC 4 |
| A5 | GPIO 23 | I2C SCL / Analog | I2C or ADC 5 |

#### Power Pins

| Pin | Function | Voltage/Current |
|-----|----------|-----------------|
| 5V | 5V output | From USB, ~500mA max |
| 3.3V | 3.3V regulated | ~150mA max |
| VIN | Voltage input | 7-12V (if not USB powered) |
| GND | Ground | Multiple pins |
| IOREF | I/O reference | 3.3V (logic level) |
| RESET | Reset line | Active low |

### On-Board LEDs

| LED | GPIO | Function |
|-----|------|----------|
| LED0 (RGB Red) | GPIO 22 | User LED / PWM |
| LED1 (RGB Green) | GPIO 19 | User LED / PWM |
| LED2 (RGB Blue) | GPIO 21 | User LED / PWM |
| TX LED | Internal | UART TX activity |
| RX LED | Internal | UART RX activity |

---

## Power Architecture

### Power Rails

```
USB 5V Input
     │
     ├─► 5V Rail ──► Shield 5V pin
     │
     ├─► 3.3V Regulator ──► 3.3V Rail
     │                        │
     │                        ├─► RISC-V Core
     │                        ├─► ESP32
     │                        ├─► Flash
     │                        ├─► Peripherals
     │                        └─► Shield 3.3V pin
     │
     └─► USB Power Management
```

### Power Consumption

| Component | Typical | Max |
|-----------|---------|-----|
| RISC-V E31 | 50mA | 100mA @ 320MHz |
| ESP32 | 80mA | 250mA (WiFi TX) |
| LCD Shield | 100mA | 300mA (backlight) |
| **Total** | **230mA** | **650mA** |

**⚠️ Important:** USB 2.0 provides max 500mA. With LCD shield, you need:
- Powered USB hub, OR
- External 5V power supply

---

## Programming Interfaces

### 1. JTAG (RISC-V)

**Protocol:** IEEE 1149.1 JTAG
**Interface:** SEGGER J-Link OB
**Speed:** Up to 4 MHz
**Features:**
- Debug (breakpoints, single-step)
- Flash programming
- Memory inspection
- Core register access

**Tools:**
- OpenOCD
- GDB (riscv64-unknown-elf-gdb)

### 2. UART (ESP32)

**Protocol:** UART 115200 8N1
**Interface:** USB CDC ACM
**Features:**
- Console access
- Firmware upload (bootloader)
- AT command interface

**Tools:**
- screen, minicom
- esptool.py

### 3. Mass Storage

**Interface:** USB MSC (Mass Storage Class)
**Size:** 10.7MB
**Usage:** Drag-and-drop firmware flashing
**Format:** FAT16

**Process:**
1. Copy .hex file to mounted drive
2. Board automatically flashes and resets
3. Mass storage remounts with new firmware

---

## Flash Organization

```
0x2000_0000 ├─ Bootloader (first stage)
            │   Size: ~8KB
            │
0x2000_2000 ├─ User Application
            │   Size: Up to ~4MB
            │   Your code goes here
            │
            └─ End of flash
```

**Boot Process:**
1. Power on → Mask ROM runs first-stage bootloader
2. First-stage initializes SPI flash
3. Second-stage bootloader loads from flash
4. User application starts at 0x2000_0000

---

## References

- **SiFive E31 Manual:** https://sifive.cdn.prismic.io/sifive/3d777659-8c4f-465a-829e-73e4bfe157b1_sifive-e31-manual-v19.08.pdf
- **HiFive1 Schematics:** https://github.com/sifive/freedom-e310-arty
- **ESP32 Datasheet:** https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- **SEGGER J-Link:** https://www.segger.com/products/debug-probes/j-link/
- **OpenOCD RISC-V:** http://openocd.org/doc/html/Debug-Adapter-Configuration.html

---

**Document Version:** 1.0  
**Last Updated:** February 9, 2026

---

## Critical System Configuration (Feb 2026 Update)

### Clock Frequency
The onboard HFROSC (High Frequency Ring Oscillator) is uncalibrated.
- **Measured Frequency:** 18.125 MHz
- **UART Divisor (115200):** 157

### GPIO Initialization
Bootloader settings may be lost during JTAG debugging. Firmware **must** explicitly configure GPIOs:
- **UART0:** Enable IOF0 on GPIO 16 & 17
- **UART1:** Enable IOF0 on GPIO 18 & 23
