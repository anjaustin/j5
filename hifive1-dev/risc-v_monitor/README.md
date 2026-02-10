# RISC-V CfC Monitor (HiFive1 Rev B)

This project implements a Closed-form Continuous-time (CfC) neural network on the SiFive HiFive1 Rev B to detect anomalies in telemetry streamed from the onboard ESP32.

## System Configuration

*   **Platform**: SiFive HiFive1 Rev B (FE310-G002)
*   **Clock Frequency**: ~18.125 MHz (HFROSC uncalibrated)
*   **Baud Rate**: 115200
*   **UART Divisor**: 157 (calculated for 18.125 MHz)

## Critical Hardware Details

### GPIO Configuration
The UART pins are **not** enabled by default after a `reset halt`. The firmware explicitly enables IOF0 on the following pins:
*   **UART0 (Console)**: GPIO 16 (RX), GPIO 17 (TX)
*   **UART1 (Telemetry)**: GPIO 18 (TX), GPIO 23 (RX)

### Register Offsets (FE310-G002)
*   **UART_DIV**: Offset `0x18` (e.g., `0x10013018`)
*   **GPIO_IOF_EN**: Offset `0x38`
*   **GPIO_IOF_SEL**: Offset `0x3C`

## Build and Run

Prerequisites: `riscv32-esp-elf-gcc` and `openocd`.

### 1. Build
```bash
make
# Convert to binary for RAM loading
~/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-objcopy -O binary riscv32imac-unknown-none-elf.elf riscv32imac-unknown-none-elf.bin
```

### 2. Run (RAM Mode)
This method loads the binary directly to ITIM RAM (0x80000000) and executes it without flashing.

```bash
# Start OpenOCD
openocd -f board/sifive-hifive1-revb.cfg &

# Load and Run via GDB
riscv32-esp-elf-gdb -ex "target extended-remote localhost:3333" \
    -ex "monitor reset halt" \
    -ex "load" \
    -ex "set \$pc=0x80000000" \
    -ex "continue" \
    riscv32imac-unknown-none-elf.elf
```

## Telemetry Format
The system expects 12-byte packets from UART1:
*   `[0]`: Magic Byte (0xAA)
*   `[1-4]`: Timestamp (uint32)
*   `[5-8]`: Free Heap (uint32)
*   `[9]`: CPU Load (uint8)
*   `[10-11]`: TX Rate (uint16)

## Expected Output
```
========================================
  RISC-V Guardian v1.0
  ESP32 Anomaly Detector
  Clock: ~18.125 MHz
========================================

Waiting for telemetry from ESP32...
```
