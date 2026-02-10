# HiFive1 Cross-Processor CfC Anomaly Detection

## System Overview

```
┌─────────────────────────────────────────────────────────┐
│                    HiFive1 Rev B01                      │
│   (FE310-G002 @ 18.125 MHz)                             │
│                                                          │
│   ┌──────────────┐          ┌──────────────────────┐   │
│   │   ESP32-S0WD │◄────────││   RISC-V E31       │   │
│   │              │ UART1 TX ││   (monitor)        │   │
│   │ - Streaming  │ (GPIO10) ││   - CfC Network   │   │
│   │ - 100Hz     │    ↓     ││   - Anomaly Det   │   │
│   │ - Load gen  │ (GPIO23) ││   - RAM Execution │   │
│   └──────────────┘                 │                    │
│         │                         │ UART0 (GPIO17)    │
│         ▼                         ▼                    │
│   /dev/ttyACM1             /dev/ttyACM0               │
│   (USB CDC)                (USB CDC)                  │
└─────────────────────────────────────────────────────────┘
```

## Firmware Status

| Processor | File | Address | Status |
|-----------|------|---------|--------|
| ESP32 | `esp32_stream/build/esp32_stream.bin` | Flash | Flashed ✓ |
| RISC-V | `risc-v_monitor/riscv32imac-unknown-none-elf.bin` | 0x80000000 | Ready (RAM) |

## Quick Start

### 1. Build and Run RISC-V Monitor
The RISC-V code runs from RAM for rapid development. See `TROUBLESHOOTING.md` for details on why.

```bash
cd risc-v_monitor
make
# Auto-convert to binary and load via OpenOCD
./run_ram.sh
```
*(Note: You need to create `run_ram.sh` or use the commands in README.md)*

### 2. Monitor Output
In a separate terminal:
```bash
python3 -c "import serial; s=serial.Serial('/dev/ttyACM0',115200); print(s.read(1000).decode())"
```

Expected:
```
========================================
  RISC-V Guardian v1.0
  ESP32 Anomaly Detector
  Clock: ~18.125 MHz
========================================
Waiting for telemetry from ESP32...
```

## Hardware Setup
*   **RISC-V Clock**: Measured at 18.125 MHz (Divisor 157 for 115200 baud).
*   **Wiring**: ESP32 GPIO 10 (UART1 TX) must be connected to HiFive1 GPIO 23 (UART1 RX).

## Troubleshooting
If you encounter "halted due to single-step", garbage UART output, or J-Link errors, **READ `TROUBLESHOOTING.md` FIRST**. It documents the critical J-Link firmware fix and clock calibration.
