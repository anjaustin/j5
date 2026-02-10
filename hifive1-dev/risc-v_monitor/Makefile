TARGET = riscv32imac-unknown-none-elf
PREFIX = /home/ztflynn/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-

CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

CFLAGS = -march=rv32imaczicsr -mabi=ilp32 -O2 -ffast-math -nostdlib -fno-builtin -I./src
LDFLAGS = -T src/memory.ld -nostdlib

SRC = src/risc-v_monitor.c src/startup.S
OBJ = src/startup.o src/risc-v_monitor.o

all: $(TARGET).elf

$(TARGET).elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/%.o: src/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET).elf

flash: $(TARGET).elf
	openocd -f interface/jlink.cfg -c "transport select jtag" \
	        -c "set WORKAREAMAX 0x4000" \
	        -f target/esp32_riscv.cfg \
	        -c "program $(TARGET).elf verify reset exit" 2>/dev/null || \
	openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg \
	        -c "adapter speed 1000" \
	        -f target/esp32_riscv.cfg \
	        -c "program $(TARGET).elf verify reset exit"

.PHONY: all clean flash
