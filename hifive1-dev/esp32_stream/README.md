# ESP32 Streaming Telemetry

Streams real-time system metrics to RISC-V via UART1.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                         ESP32                          │
│  ┌─────────────┐      ┌────────────────────────────┐  │
│  │ esp_timer  │ ───► │ UART1 TX (GPIO 10)         │  │
│  │ 10ms ISR   │      │ 115200 baud                │  │
│  └─────────────┘      └────────────────────────────┘  │
│                             │                           │
│                             ▼                           │
│                     [Physical Wire]                    │
│                             ▼                           │
│                      HiFive1 GPIO 23                   │
│                      (UART1 RX)                        │
└─────────────────────────────────────────────────────────┘
```

## Critical Wiring Note
The ESP32 uses **GPIO 10** for UART1 TX. This pin must be connected to the RISC-V's **UART1 RX** pin, which is **GPIO 23** on the HiFive1 header.

*   **ESP32 TX (GPIO 10)** ----> **HiFive1 RX (GPIO 23)**

## Data Format (12 bytes per sample)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 1 | magic | 0xAA |
| 1 | 4 | timestamp_ms | Milliseconds since boot |
| 5 | 4 | free_heap | Available heap in bytes |
| 9 | 1 | cpu_load | CPU load percentage |
| 10 | 2 | tx_rate | UART TX rate (bytes/sec) |

## Build and Flash

```bash
cd /home/ztflynn/j5/hifive1-dev/esp32_stream
idf.py build
idf.py -p /dev/ttyACM1 flash monitor
```
