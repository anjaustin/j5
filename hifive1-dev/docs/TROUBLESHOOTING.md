# HiFive1 Rev B Debugging & Troubleshooting Log

This document records the critical issues encountered and resolved during the development of the RISC-V CfC Monitor.

## 1. J-Link Firmware Bug (The "6-Hour Battle")

**Issue:** OpenOCD would connect to the target but fail to resume execution. Commands like `resume` or `step` resulted in `riscv.cpu.0 halted due to single-step` or `undefined debug reason`.

**Root Cause:** The J-Link OB firmware on the HiFive1 board (dated ~July 2021) had a regression in RISC-V DMI (Debug Module Interface) handling. It could write to memory but failed to release the core from debug mode.

**Fix:**
1.  Locate the Segger J-Link tools for ARM/Linux (found in `~/JLink_Linux_V916a_arm/`).
2.  **CRITICAL:** The tools are 32-bit ARM binaries. On an x86_64 system, they require `qemu-arm-static`.
3.  **CRITICAL:** Ensure `binfmt_misc` is configured correctly. We found a conflict where QEMU was hijacking execution. Fixed by resetting binfmt.
4.  Run `JLinkExe` to update firmware:
    ```bash
    sudo LD_LIBRARY_PATH=. qemu-arm-static ./JLinkExe
    ```
    This updated the firmware to **Nov 7 2022**, which fixed the DMI resume bug.

## 2. Clock Frequency & UART Baud Rate

**Issue:** UART output was garbage characters (`d\xaeR...`) despite configuring the baud rate divisor for the standard 16 MHz clock.

**Investigation:**
We wrote a small assembly program (`src/test_clock.S`) to measure the CPU cycle count (`mcycle`) against the RTC `mtime` (32.768 kHz).

**Result:**
*   Measured Frequency: **18,125,200 Hz (18.125 MHz)**
*   Standard Assumption: 16 MHz
*   The HFROSC (High Frequency Ring Oscillator) is uncalibrated by default.

**Fix:**
Recalculated UART divisor for 115200 baud:
*   `18125200 / 115200 - 1 = 157`
*   Updated `RISC-V_monitor.c` to use `DIV = 157`.

## 3. GPIO Configuration (The "Silent UART")

**Issue:** Even with the correct baud rate, no output appeared after a hard reset.

**Root Cause:**
On the FE310-G002, GPIO pins default to **Input** (Safe state) after reset. They must be explicitly switched to "IOF" (Input/Output Function) mode to connect them to the UART peripheral. The bootloader usually does this, but `reset halt` or loading directly to RAM bypasses the bootloader logic.

**Fix:**
Added `gpio_init()` to explicitly enable IOF0 for:
*   **UART0 (Console):** GPIO 16 (RX), 17 (TX)
*   **UART1 (Telemetry):** GPIO 18 (TX), 23 (RX)

## 4. Execution Model (RAM vs Flash)

**Issue:** Flashing to SPI Flash (0x20000000) was unreliable and slow during debugging. OpenOCD often failed to verify or set the PC correctly.

**Fix:**
Switched to **RAM Load** workflow:
1.  Link code to `0x80000000` (ITIM RAM, 16KB).
2.  Convert ELF to Binary (`objcopy -O binary`).
3.  Load binary directly via OpenOCD:
    ```bash
    load_image monitor.bin 0x80000000 bin
    resume 0x80000000
    ```
This provides instant load-and-run capability, essential for rapid iteration.

## 5. UART Register Map

**Issue:** Original code used incorrect offsets for the FE310-G002 UART.
**Fix:**
Corrected offsets:
*   `TXDATA`: 0x00
*   `RXDATA`: 0x04
*   `TXCTRL`: 0x08
*   `RXCTRL`: 0x0C
*   `DIV`:    0x18  <-- Critical fix (was using 0x08)
