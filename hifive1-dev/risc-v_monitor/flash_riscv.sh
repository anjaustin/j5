#!/bin/bash
# Flash RISC-V on HiFive1 using OpenOCD

ELF_FILE="riscv32imac-unknown-none-elf.elf"
OPENOCD="/usr/bin/openocd"

echo "Flashing RISC-V on HiFive1..."
echo "ELF file: $ELF_FILE"

# Try different OpenOCD configurations
for cfg in \
    "interface/jlink.cfg -c 'transport select jtag' -c 'adapter speed 2000' -c 'jtag newtap e31 cpu -irlen 5 -expected-id 0x20000913' -c 'target create e31.cpu riscv -chain-position e31.cpu'" \
    "interface/jlink.cfg -c 'transport select jtag' -c 'adapter speed 1000'"; do
    
    echo "Trying: $cfg"
    
done

# Direct approach using telnet
echo ""
echo "Starting OpenOCD server in background..."
$OPENOCD -f interface/jlink.cfg \
    -c "transport select jtag" \
    -c "adapter speed 2000" \
    -c "jtag newtap e31 cpu -irlen 5 -expected-id 0x20000913" \
    -c "target create e31.cpu riscv -chain-position e31.cpu" \
    -c "e31.cpu configure -work-area-phys 0x80000000 -work-area-size 0x4000" \
    -c "init" \
    -c "halt" \
    -c "flash write_image erase $ELF_FILE elf" \
    -c "reset run" \
    -c "shutdown" 2>&1

echo ""
echo "Flash complete."
