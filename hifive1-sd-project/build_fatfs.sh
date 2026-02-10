#!/bin/bash

PREFIX=/home/ztflynn/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-

echo "Building FatFs SD Card Test..."

# Compile stdlib support
echo "  - Compiling stdlib_support.c..."
${PREFIX}gcc -march=rv32imaczicsr -mabi=ilp32 -O2 -ffast-math -nostdlib -fno-builtin \
    -I. -c -o stdlib_support.o stdlib_support.c

# Compile FatFs
echo "  - Compiling ff.c..."
${PREFIX}gcc -march=rv32imaczicsr -mabi=ilp32 -O2 -ffast-math -nostdlib -fno-builtin \
    -I. -c -o source/ff.o source/ff.c

# Compile diskio
echo "  - Compiling diskio.c..."
${PREFIX}gcc -march=rv32imaczicsr -mabi=ilp32 -O2 -ffast-math -nostdlib -fno-builtin \
    -I. -c -o diskio.o diskio.c

# Compile test program
echo "  - Compiling fatfs_test.c..."
${PREFIX}gcc -march=rv32imaczicsr -mabi=ilp32 -O2 -ffast-math -nostdlib -fno-builtin \
    -I. -c -o fatfs_test.o fatfs_test.c

# Compile startup
echo "  - Compiling startup..."
${PREFIX}gcc -march=rv32imaczicsr -mabi=ilp32 -O2 -nostdlib \
    -c -o crt0.o crt0.s

# Link
echo "  - Linking..."
${PREFIX}ld -T ram.ld -nostdlib -o fatfs_test.elf \
    crt0.o fatfs_test.o source/ff.o diskio.o stdlib_support.o

echo ""
echo "Build complete: fatfs_test.elf"
ls -la fatfs_test.elf
