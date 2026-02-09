# Makefile for HiFive1 SD Card Test

# Toolchain
PREFIX = riscv64-unknown-elf
CC = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
SIZE = $(PREFIX)-size

# Target
TARGET = hifive1_sd_test

# Compiler flags
CFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medlow
CFLAGS += -O0 -g -Wall -Wextra
CFLAGS += -nostdlib -nostartfiles
CFLAGS += -ffreestanding

# Linker flags
LDFLAGS = -T hifive1.ld
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections

# Source files
OBJS = $(TARGET).o crt0.o

# Default target
all: $(TARGET).hex $(TARGET).lst

# Compile C files
$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
crt0.o: crt0.s
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(OBJS) hifive1.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

# Generate hex file for flashing
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Generate binary file
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Generate listing file
$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d $< > $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).lst

# Flash to HiFive1 via JTAG
flash: $(TARGET).hex
	openocd -f board/sifive-hifive1-revb.cfg -c "init; halt; flash write_image erase $(TARGET).hex; reset; exit"

# Flash via drag-and-drop
flash-dragdrop: $(TARGET).hex
	cp $(TARGET).hex /media/ztflynn/HiFive/

# Monitor serial output
monitor:
	@echo "screen /dev/ttyACM1 115200"

.PHONY: all clean flash flash-dragdrop monitor
