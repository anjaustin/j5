# HiFive1 Cross-Processor CfC Anomaly Detection

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    HiFive1 Rev B01                        │
│                                                          │
│   ESP32 (stream) ──UART1──► RISC-V (CfC)              │
│        │                             │                  │
│        ▼                             ▼                  │
│   /dev/ttyACM1                 /dev/ttyACM0             │
│   (streaming data)              (debug output)          │
└─────────────────────────────────────────────────────────┘
```

## Firmware Status

| Processor | Location | Size | Status |
|-----------|----------|------|--------|
| ESP32 | `esp32_stream/build/esp32_stream.bin` | 186KB | ✓ Flashed |
| RISC-V | `risc-v_monitor/riscv32imac-unknown-none-elf.elf` | 7.5KB | ⚠ Build OK |

## ESP32 Firmware (Flashed ✓)

**Features:**
- Streams telemetry @ 100Hz via UART1
- Timer-driven, no polling
- Built-in load generator for perturbation testing
- 12 bytes/sample: magic|timestamp|heap|cpu|txrate

**To verify ESP32 is working:**
```bash
# ESP32 UART0 output (debug)
python3 -c "import serial; s=serial.Serial('/dev/ttyACM1',115200,timeout=3); print(s.read(5000).decode()); s.close()"
```

## RISC-V Firmware (Build OK)

**Features:**
- UART1 interrupt-driven receiver
- 64-neuron CfC neural network
- Hebbian online learning
- Anomaly detection (NORMAL/WARN/ANOMALY)

## Flashing RISC-V

The HiFive1 RISC-V core requires OpenOCD with specific configuration:

### Option 1: Using OpenOCD with J-Link
```bash
cd /home/ztflynn/j5/hifive1-dev/risc-v_monitor
make flash
```

### Option 2: Manual OpenOCD
```bash
openocd -f interface/jlink.cfg \
        -c "transport select jtag" \
        -c "adapter speed 2000" \
        -f target/sifive_e31.cfg \
        -c "program riscv32imac-unknown-none-elf.elf verify reset exit"
```

### Option 3: Using espressif's RISC-V tool
```bash
# Install Freedom E SDK
git clone https://github.com/sifive/freedom-e-sdk
cd freedom-e-sdk
make BOARD=hifive1 SOFTWARE=risc-v_monitor
openocd -f bsp/drivers/jlink/openocd.cfg -c "program build/risc-v_monitor/debug/risc-v_monitor verify reset"
```

## Test Procedure

1. **Power on HiFive1** - Both processors should be running
2. **Verify ESP32 streaming** - ESP32 UART0 output on /dev/ttyACM1
3. **Flash RISC-V** - Using OpenOCD commands above
4. **Monitor RISC-V output** - /dev/ttyACM0 should show:
```
========================================
  RISC-V Guardian v1.0
  ESP32 Anomaly Detector
========================================
Receiving from ESP32 UART1...
Running CfC anomaly detection.

[100] H:14A000 C:5 T:1200 -> NORMAL
[200] H:14A000 C:7 T:1250 -> NORMAL
[300] H:14A000 C:85 T:4500 -> ANOMALY!
```

## Perturbation Test

The ESP32 load generator ramps up CPU usage:
- 0-10s: idle (CPU ~5%)
- 10-20s: light load (CPU ~30%)  
- 20-30s: medium load (CPU ~60%)
- 30s+: heavy load (CPU ~90%)

RISC-V CfC should detect ANOMALY when CPU > 80%.

## Project Structure

```
/home/ztflynn/j5/hifive1-dev/
├── esp32_stream/                    # ESP32 streaming telemetry
│   ├── main/esp32_stream.c         # Timer ISR @ 100Hz
│   ├── build/esp32_stream.bin       # Flashed firmware
│   └── CMakeLists.txt
│
├── risc-v_monitor/                  # RISC-V anomaly detector  
│   ├── src/risc-v_monitor.c        # UART1 ISR + CfC
│   ├── riscv32imac-unknown-none-elf.elf
│   ├── Makefile
│   └── sifive_e31.cfg              # OpenOCD config
│
└── PROJECT_SUMMARY.md              # This file
```

## Building

### ESP32
```bash
cd esp32_stream
. ~/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyACM1 flash
```

### RISC-V
```bash
cd risc-v_monitor
make clean && make
```
