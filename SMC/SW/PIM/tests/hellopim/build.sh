#!/bin/bash

load_model system/bare_metal_link_arm7.sh > test.ld

echo ">>> Compiling the boot loader ..."
${PIM_CROSS_COMPILE}gcc -mfloat-abi=softfp -march=armv7-a -fno-builtin -nostdinc -o boot.o -c boot.S
echo ">>> Compiling the execution kernel ..."
${PIM_CROSS_COMPILE}gcc -c -march=armv7-a -g test.c -o test.s -mno-thumb-interwork -fno-stack-protector -S
${PIM_CROSS_COMPILE}gcc -c -march=armv7-a -g test.c -o test.o -mno-thumb-interwork -fno-stack-protector
echo ">>> Linking ..."
${PIM_CROSS_COMPILE}ld -T test.ld test.o boot.o -lc -lgcc -lgcc_eh -o test.elf -non_shared -static  \
	--library-path /usr/arm-linux-gnueabihf/lib/ \
 	--library-path /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/

echo ">>> Dumping symbol table ..."
get_symbol_addr PIM_DEBUG_REG test.elf
export PIM_ERROR_REG=0x0
export PIM_INTR_REG=0x0
export PIM_COMMAND_REG=0x0
export PIM_STATUS_REG=0x0
