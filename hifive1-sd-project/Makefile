# Makefile for HiFive1 Development

# Toolchain
PREFIX = riscv64-unknown-elf
CC = $(PREFIX)-gcc
AS = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
SIZE = $(PREFIX)-size

# Compiler flags
CFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medlow
CFLAGS += -O0 -g -Wall -Wextra
CFLAGS += -nostdlib -nostartfiles
CFLAGS += -ffreestanding

# Assembler flags
ASFLAGS = -march=rv32imac -mabi=ilp32

# Linker flags
LDFLAGS = -T hifive1.ld
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections

# ============================================================================
# SD Card Test Target
# ============================================================================
TARGET = hifive1_sd_test
OBJS = $(TARGET).o crt0.o

sd_test: $(TARGET).hex $(TARGET).lst

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) hifive1.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d $< > $@

flash-sd-test: $(TARGET).hex
	openocd -f board/sifive-hifive1-revb.cfg -c "init; halt; flash write_image erase $(TARGET).hex; reset; exit"

# ============================================================================
# ESP32 Reset Utility - Puts ESP32 into UART bootloader mode
# ============================================================================
ESP32_RESET = esp32_reset
ESP32_OBJS = $(ESP32_RESET).o crt0.o

esp32_reset: $(ESP32_RESET).hex $(ESP32_RESET).lst

$(ESP32_RESET).o: $(ESP32_RESET).c
	$(CC) $(CFLAGS) -c $< -o $@

$(ESP32_RESET).elf: $(ESP32_OBJS) hifive1.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(ESP32_OBJS) -o $@
	$(SIZE) $@

$(ESP32_RESET).hex: $(ESP32_RESET).elf
	$(OBJCOPY) -O ihex $< $@

$(ESP32_RESET).bin: $(ESP32_RESET).elf
	$(OBJCOPY) -O binary $< $@

$(ESP32_RESET).lst: $(ESP32_RESET).elf
	$(OBJDUMP) -d $< > $@

flash-esp32-reset: $(ESP32_RESET).hex
	openocd -f board/sifive-hifive1-revb.cfg -c "init; halt; flash write_image erase $(ESP32_RESET).hex; reset; exit"

# ============================================================================
# Common Targets
# ============================================================================

# Clean
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).lst
	rm -f $(ESP32_OBJS) $(ESP32_RESET).elf $(ESP32_RESET).hex $(ESP32_RESET).bin $(ESP32_RESET).lst

# Flash via drag-and-drop
flash-dragdrop: $(TARGET).hex
	cp $(TARGET).hex /media/ztflynn/HiFive/

# Monitor serial output
monitor:
	@echo "screen /dev/ttyACM1 115200"

.PHONY: sd_test esp32_reset clean flash-sd-test flash-esp32-reset flash-dragdrop monitor

# ============================================================================
# UART Test
# ============================================================================
UART_TEST = uart_test
UART_OBJS = $(UART_TEST).o crt0.o

uart_test: $(UART_TEST).hex $(UART_TEST).lst

$(UART_TEST).o: $(UART_TEST).c
	$(CC) $(CFLAGS) -c $< -o $@

$(UART_TEST).elf: $(UART_OBJS) hifive1.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(UART_OBJS) -o $@
	$(SIZE) $@

$(UART_TEST).hex: $(UART_TEST).elf
	$(OBJCOPY) -O ihex $< $@

$(UART_TEST).bin: $(UART_TEST).elf
	$(OBJCOPY) -O binary $< $@

$(UART_TEST).lst: $(UART_TEST).elf
	$(OBJDUMP) -d $< > $@

flash-uart-test: $(UART_TEST).hex
	openocd -f board/sifive-hifive1-revb.cfg -c "init; halt; flash write_image erase $(UART_TEST).hex; reset; exit"
