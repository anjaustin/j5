# ESP32 Development Guide

**Last Updated:** February 9, 2026

---

## Overview

This project implements **CfC (Closed-form Continuous-time)** neural networks on the ESP32-S0WD, inspired by [The Reflex](https://github.com/EntroMorphic/the-reflex) project.

**Key Achievements:**
- ✅ POPCOUNT-based neural inference (no multiply, no floating point)
- ✅ 16-neuron CfC layer running on ESP32-S0WD
- ✅ Direct UART output at 115200 baud
- ✅ Direct register access (no ESP-IDF UART driver overhead)

---

## UART Communication

**Critical: UART baud rates differ by phase**

| Phase | Baud Rate | Port | Notes |
|-------|-----------|------|-------|
| Boot ROM | **74880** | `/dev/ttyACM1` | Encrypted/obfuscated boot log |
| App Runtime | **115200** | `/dev/ttyACM1` | Human-readable output |

### Reading Output

```bash
# Boot log (74880 baud - garbled binary)
python3 -c "import serial; s=serial.Serial('/dev/ttyACM1',74880,timeout=3); print(s.read(1000)); s.close()"

# App output (115200 baud - readable)
python3 -c "import serial; s=serial.Serial('/dev/ttyACM1',115200,timeout=5); print(s.read(5000)); s.close()"
```

---

## CfC Neural Network

### Architecture

```
Input (16 bits) → [POPCOUNT + LUT] → Output (16 binary activations)
```

### The Math

For each neuron (one per bit position):
```
pre_act = popcount(input & pos_mask) - popcount(input & neg_mask)
output  = sigmoid(pre_act) >= sigmoid(0)
```

### Memory Layout

```c
typedef struct {
    uint32_t pos_mask[1];  // Bits where weight = +1
    uint32_t neg_mask[1];   // Bits where weight = -1
} cfc_weights_t;
```

### Key Operations

| Operation | Implementation |
|-----------|----------------|
| POPCOUNT | `__builtin_popcount()` (hardware instruction) |
| Sigmoid | Lookup table (33 entries for [-16, +16]) |
| Threshold | `sigmoid_lut[16]` = sigmoid(0) = midpoint |

---

## Direct UART Register Access

### ESP32 UART0 Registers

```c
#define UART0_BASE    0x3FF40000
#define UART0_TXFIFO  (UART0_BASE + 0x00)
#define UART0_STAT    (UART0_BASE + 0x1C)
```

### UART Output Code

```c
volatile uint32_t *txfifo = (volatile uint32_t *)0x3FF40000;
volatile uint32_t *stat   = (volatile uint32_t *)0x3FF4001C;

void uart_putchar(char c) {
    while (*stat & (1 << 23));  // Wait for TXFIFO not full
    *txfifo = (uint8_t)c;
}
```

### TX FIFO Status Bit

| Bit | Name | Meaning |
|-----|------|---------|
| 23 | TXFIFO_FULL | 1 = TX FIFO is full |

---

## Direct Register Pattern

### Template

```c
// Define base address and offsets
#define PERIPH_BASE   0x3FF00000
#define PERIPH_REG    (PERIPH_BASE + OFFSET)

// Access as volatile pointers
volatile uint32_t *reg = (volatile uint32_t *)PERIPH_REG;

// Read
uint32_t val = *reg;

// Write
*reg = value;
```

### Common Patterns

```c
// Set bits (OR)
*REG |= (1 << BIT);

// Clear bits (AND with inverted)
*REG &= ~(1 << BIT);

// Toggle bits (XOR)
*REG ^= (1 << BIT);

// Wait for condition
while (*REG & COND) { }

// Set with mask
*REG = (value << SHIFT) & MASK;
```

---

## ESP32-S0WD Specifications

| Feature | Value |
|---------|-------|
| **Architecture** | Xtensa LX6 (single-core) |
| **Clock** | 240 MHz |
| **RAM** | 520 KB |
| **Flash** | 4 MB |
| **WiFi** | 802.11b/g/n |
| **Bluetooth** | BT/BLE |

### Peripheral Base Addresses

| Peripheral | Base Address |
|-----------|--------------|
| UART0 | 0x3FF40000 |
| UART1 | 0x3FF50000 |
| GPIO | 0x3FF44000 |
| TIMG0 | 0x3FF5F000 |
| PCNT | 0x3FF57000 |

---

## CfC Demo Output Example

```
========================================
  CfC Neural Network Demo for ESP32-S0WD
  Closed-form Continuous-time (POPCOUNT only)
========================================

Pattern: 0b1010101010101010 (0xAAAA)
16 neurons, 1 neuron per bit position

Input:  0b0000000000000000 (0x00000000) -> ################ (16/16)
Input:  0b0000000000000001 (0x00000001) -> .############### (15/16)
Input:  0b0000000000000010 (0x00000002) -> ################ (16/16)
Input:  0b0000000000000101 (0x00000005) -> .#.############# (14/16)
```

---

## Building and Flashing

```bash
cd hifive1-dev/esp32_display

# Setup ESP-IDF
. ~/esp-idf/export.sh

# Build
idf.py build

# Flash
idf.py -p /dev/ttyACM1 flash

# Monitor (115200 baud)
idf.py -p /dev/ttyACM1 monitor
```

---

## Related Documentation

- [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - One-page cheat sheet
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues
- [ARCHITECTURE.md](ARCHITECTURE.md) - Hardware architecture
- [reflex-reference/](../reflex-reference/) - The Reflex project reference

---

**Inspired by:** [The Reflex](https://github.com/EntroMorphic/the-reflex) - Sub-microsecond robotics coordination using cache coherency.
