#!/bin/bash

if [ $ARCH == arm64 ]; then
	pim_arch=64
elif [ $ARCH == arm ]; then
	pim_arch=32
else
	print_err "Illegal ARCH!"
	exit
fi

echo -e "
#define PHY_BASE $PIM_ADDRESS_BASE
#define PHY_SIZE $(conv_to_bytes $PIM_ADDRESS_SIZE)
#define PIM_SLICETABLE 	$PIM_SLICETABLE 
#define PIM_SLICECOUNT	$PIM_SLICECOUNT
#define PIM_SLICEVSTART $PIM_SLICEVSTART
#define PIM_M5_REG	$PIM_M5_REG
#define PIM_M5_D1_REG	$PIM_M5_D1_REG
#define PIM_M5_D2_REG	$PIM_M5_D2_REG
#define PIM_HOST_OFFSET	$PIM_HOST_OFFSET
#define pim_arch $pim_arch
$(set_if_true $DEBUG_PIM_DRIVER "#define DEBUG_DRIVER")
$(set_if_true $PIM_MERGE_SLICES "#define PIM_MERGE_SLICES")
" > _params.h

echo -e "
CROSS_COMPILE := $HOST_CROSS_COMPILE
COMPILED_KERNEL_PATH := $COMPILED_KERNEL_PATH
ARCH := $ARCH
" > Makefile

cat Makefile.mk >> Makefile
make
chmod +x ins.sh
