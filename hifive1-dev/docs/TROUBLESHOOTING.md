# Troubleshooting Guide
## HiFive1 Rev B01 Development

---

## Quick Diagnostics

### Check if Board is Connected
```bash
# USB device present?
lsusb | grep -i "1366\|segger\|hifive"
# Expected: Bus 001 Device 004: ID 1366:1061 SEGGER HiFive

# Serial ports available?
ls /dev/ttyACM*
# Expected: /dev/ttyACM0  /dev/ttyACM1

# Mass storage mounted?
ls /media/$USER/HiFive/
# Expected: Board Info.txt  Readme.html
```

---

## Common Issues

### 1. Board Not Detected (No USB Device)

**Symptoms:**
- `lsusb` shows no SEGGER device
- No `/dev/ttyACM*` devices
- Green LEDs on board are on

**Causes:**

#### A. Power-Only USB Cable (Most Common)

**Check:**
```bash
# Test cable with phone
# Connect phone to Pi with same cable
lsusb | grep -i phone
# If phone doesn't show up, cable is power-only
```

**Fix:**
- Use a different USB cable (test with phone first)
- Look for cables labeled "data + charge"
- Avoid cables that came with chargers only

#### B. USB 3.0 Compatibility Issues

**Symptoms:**
- Works on USB 2.0 but not USB 3.0 (blue ports)

**Fix:**
- Try USB 2.0 ports (black ports on Pi 4)
- Use a USB 2.0 hub

#### C. Power Supply Issues

**Symptoms:**
- Board briefly appears then disappears
- `dmesg` shows undervoltage warnings

**Check:**
```bash
vcgencmd get_throttled
# 0x50000 = undervoltage detected
```

**Fix:**
- Use official Raspberry Pi power supply (5.1V, 3A)
- Use powered USB hub
- Don't power other devices from Pi USB ports

---

### 2. Board Detected But Serial Not Working

**Symptoms:**
- `lsusb` shows SEGGER device
- `/dev/ttyACM*` exists
- Can't read from serial (`cat /dev/ttyACM1` shows nothing or garbage)

#### A. Permission Denied

**Symptoms:**
```
cat: /dev/ttyACM1: Permission denied
```

**Fix:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and back in (or reboot)
# Verify:
groups $USER | grep dialout
```

#### B. ESP32 Boot Messages

**Symptoms:**
Seeing ESP32 boot messages instead of RISC-V output:
```
rst:0x1 (POWERON_RESET),boot:0x3a (SPI_FAST_FLASH_BOOT)
flash read err, 1000
ets_main.c 371
```

**Explanation:**
This is **NORMAL**! The HiFive1 Rev B has two processors:
- `/dev/ttyACM0` → JTAG interface (RISC-V debugging)
- `/dev/ttyACM1` → UART connected to ESP32

The ESP32 boots first and outputs these messages.

**Access RISC-V:**
Use JTAG interface, not serial:
```bash
# Terminal 1: Start OpenOCD
openocd -f board/sifive-hifive1-revb.cfg

# Terminal 2: Connect GDB
riscv64-unknown-elf-gdb your_program.elf
(gdb) target remote localhost:3333
```

#### C. Wrong Baud Rate

**Symptoms:**
- Garbled output
- Random characters

**Fix:**
Default baud rate is 115200:
```bash
stty -F /dev/ttyACM1 115200
screen /dev/ttyACM1 115200
```

---

### 3. LCD Shield Causes Disconnect

**Symptoms:**
- Board works without shield
- USB disconnects when shield attached
- `dmesg` shows: `usb 1-1.x: USB disconnect`

#### A. Power Overload

**Cause:**
LCD shield draws too much current (~100-300mA), exceeding USB power limits.

**Fix - Option 1: Powered USB Hub**
```bash
# Use powered hub between Pi and HiFive1
Pi → Powered Hub → HiFive1 + Shield
```

**Fix - Option 2: External Power**
- Power shield separately if it has power jack
- Use bench power supply

**Fix - Option 3: Limit Brightness**
If you have code running on the board:
```c
// Reduce LCD backlight brightness
pwm_set_duty_cycle(BACKLIGHT_PIN, 50); // 50% instead of 100%
```

#### B. Shield Short Circuit

**Check:**
- Remove shield, check for bent pins
- Inspect shield for solder bridges
- Check shield power consumption with multimeter

---

### 4. Flashing Fails

#### A. Mass Storage Flash Not Working

**Symptoms:**
- Copied .hex file but board doesn't reset
- Old program still running

**Fix:**
```bash
# Ensure sync before eject
sync

# Check if file was actually copied
ls -la /media/$USER/HiFive/*.hex

# Wait 10 seconds for auto-flash
# Manually reset board if needed (press RESET button)
```

#### B. OpenOCD Flash Fails

**Symptoms:**
```
Error: couldn't bind telnet to socket: Address already in use
```

**Fix:**
```bash
# Kill existing OpenOCD
killall openocd

# Or find and kill specific process
ps aux | grep openocd
kill <PID>
```

#### C. Permission Denied on Mass Storage

**Symptoms:**
```
cp: cannot create regular file '/media/.../file.hex': Permission denied
```

**Fix:**
```bash
# Remount with user permissions
sudo umount /media/$USER/HiFive
sudo mount -o uid=$UID,gid=$(id -g) /dev/sda /media/$USER/HiFive

# Or just use sudo for copy
sudo cp file.hex /media/$USER/HiFive/
sudo sync
```

---

### 5. GDB Connection Issues

#### A. Can't Connect to localhost:3333

**Symptoms:**
```
(gdb) target remote localhost:3333
localhost:3333: Connection refused
```

**Causes & Fixes:**

**OpenOCD not running:**
```bash
# Check if OpenOCD is running
pgrep openocd

# Start OpenOCD
openocd -f board/sifive-hifive1-revb.cfg
```

**Port in use:**
```bash
# Find what's using port 3333
sudo lsof -i :3333

# Kill it
kill <PID>
```

**Wrong OpenOCD config:**
```bash
# Use correct board file
openocd -f board/sifive-hifive1-revb.cfg
# NOT just interface/jlink.cfg
```

#### B. GDB Hangs on "target remote"

**Symptoms:**
GDB connects but no response from board.

**Fix:**
```bash
# In GDB, try resetting
(gdb) monitor reset halt
(gdb) target remote localhost:3333
```

---

### 6. Code Doesn't Run After Flash

#### A. Entry Point Wrong

**Symptoms:**
- Flash succeeds
- Board resets
- No output or unexpected behavior

**Check:**
```bash
# Check entry point in ELF
riscv64-unknown-elf-readelf -h your_program.elf | grep Entry
# Should be: 0x20000000

# Check first instruction
riscv64-unknown-elf-objdump -d your_program.elf | head -20
```

**Fix:**
Ensure linker script has correct entry:
```ld
ENTRY(_start)
```

And startup code is at correct address:
```asm
.section .text.init
.global _start
_start:
    # Initialize stack, data, etc.
```

#### B. Clock Not Initialized

**Symptoms:**
- Code runs but very slowly
- UART output at wrong speed

**Fix:**
HiFive1 needs clock initialization (PLL setup). Use provided startup code or initialize manually:
```c
// Set up PLL for 16MHz or 256MHz
// See freedom-e-sdk for reference implementation
```

---

## Diagnostic Commands

### Check USB Power
```bash
# Check for undervoltage
vcgencmd get_throttled
# 0x0 = OK
# 0x50000 = Undervoltage detected

# Check input voltage (if available)
cat /sys/class/hwmon/hwmon*/in0_input 2>/dev/null | awk '{print "Voltage: " $1/1000 "V"}'
```

### Check USB Devices
```bash
# Detailed USB info
lsusb -v -d 1366:1061 2>&1 | grep -E "idVendor|idProduct|bcdDevice|iProduct|iManufacturer"

# USB device tree
lsusb -t

# Kernel messages
sudo dmesg | grep -iE "usb.*1366|usb.*segger|usb.*hifive"
```

### Check Serial Ports
```bash
# List serial devices
ls -la /dev/ttyACM*

# Check permissions
stat /dev/ttyACM1

# Test with stty
stty -F /dev/ttyACM1

# Read raw data
timeout 5 cat /dev/ttyACM1 | hexdump -C
```

### Check OpenOCD
```bash
# Is OpenOCD running?
pgrep -a openocd

# Test telnet connection
echo "targets" | timeout 2 telnet localhost 4444 2>/dev/null

# Check GDB port
sudo lsof -i :3333
```

### Check Flash Contents
```bash
# Via OpenOCD
timeout 10 openocd -f board/sifive-hifive1-revb.cfg -c "init; halt; mdw 0x20000000 4; exit" 2>&1 | grep "0x200000"

# Check if mass storage has our file
ls -la /media/$USER/HiFive/
hexdump -C /media/$USER/HiFive/*.hex | head -5
```

---

## Known Limitations

### Hardware
1. **USB Power:** Max 500mA from USB 2.0, 900mA from USB 3.0
   - LCD shield + HiFive1 can exceed this
   - Solution: Powered USB hub

2. **ESP32 Flash:** ESP32 on board has no dedicated flash
   - Shares flash with RISC-V or uses external flash
   - Current boot error is expected without proper firmware

3. **Shield Compatibility:** Not all Arduino shields work
   - Check 3.3V compatibility (many Arduino shields are 5V)
   - Check pin conflicts with JTAG

### Software
1. **No Standard Library:** Bare-metal environment
   - No printf, malloc, etc. by default
   - Must implement or use embedded library

2. **Limited RAM:** Only 16KB
   - Stack, heap, and data must fit
   - Careful memory management required

---

## Getting Help

### Logs to Collect

When reporting issues, include:

```bash
# USB info
lsusb > usb_info.txt

# Kernel messages
sudo dmesg > dmesg.txt

# OpenOCD log
openocd -f board/sifive-hifive1-revb.cfg -d3 > openocd_debug.txt 2>&1

# GDB session
script -q gdb_session.txt
riscv64-unknown-elf-gdb your_program.elf
(gdb) set logging on
(gdb) target remote localhost:3333
(gdb) # ... your commands ...
(gdb) quit
exit
```

### Resources

- **SiFive Forum:** https://forums.sifive.com/
- **RISC-V Forum:** https://riscv.org/forums/
- **OpenOCD Docs:** http://openocd.org/doc/html/
- **ESP32 Docs:** https://docs.espressif.com/

---

## Emergency Recovery

### If Board Won't Boot

1. **Force Bootloader Mode:**
   ```bash
   # Hold RESET button
   # Plug in USB while holding RESET
   # Release after 2 seconds
   ```

2. **Flash via JTAG:**
   ```bash
   # This works even if mass storage is corrupted
   openocd -f board/sifive-hifive1-revb.cfg \
     -c "init; halt; flash erase_sector 0 0 last; \
         program your_program.hex verify; reset; exit"
   ```

3. **Factory Reset:**
   ```bash
   # Erase entire flash and restore bootloader
   # Requires JTAG programmer or specialized tool
   ```

---

**Last Updated:** February 9, 2026  
**Version:** 1.0
