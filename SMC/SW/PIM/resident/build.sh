#!/bin/bash

# Initial checks
if [ -z $1 ] || ( ! [ $1 == "7" ] && ! [ $1 == "8" ] ); then
	print_err "resident/build.sh: first argument must be either 7 or 8 specifying the ARM architecture [$1]"
	exit
fi

if [ -z $2 ] || ( ! [ -f $PIM_SW_DIR/kernels/$2.c ] ); then
	print_msg "Building the default resident code ..."
	BUILD_RESIDENT=TRUE
	cp $PIM_SW_DIR/kernels/template.c ./kernel.c
else
	print_msg "Building the kernel:${COLOR_YELLOW} $2 $OFFLOADED_KERNEL_SUBNAME"
	cp $PIM_SW_DIR/kernels/$2.c ./kernel.c
	BUILD_RESIDENT=FALSE
fi

################################################################################################

if [ $1 == "7" ]; then export SIZEOF_ULONG=4; else export SIZEOF_ULONG=8; fi

# Parameters
echo -e "
#define PIM_VREG_SIZE $PIM_VREG_SIZE
#define PIM_SREG_COUNT $PIM_SREG_COUNT
#define SIZEOF_ULONG $SIZEOF_ULONG
$(set_if_true $DEBUG_PIM_RESIDENT "#define DEBUG_RESIDENT")
" > _params.h

# Kernel parameters
echo -e "
#define $OFFLOADED_KERNEL_NAME
#define XFER_SIZE $PIM_DMA_XFER_SIZE
#define MAX_XFER_SIZE $PIM_DMA_MAX_XFER_SIZE
#define kernel_$OFFLOADED_KERNEL_SUBNAME
$(set_if_true $USE_HMC_ATOMIC_CMD "#define USE_HMC_ATOMIC_CMD")
" > ./kernel_params.h

load_model system/pim_bare_metal_link.sh > resident.ld

################################################################################################
if [ $1 == "7" ]; then	# PIM is ARMv7

	print_msg " > Boot loader ..."
	${PIM_CROSS_COMPILE}gcc -mfloat-abi=softfp -march=armv7-a -fno-builtin -nostdinc -o boot.o -c boot_arm7.S -Wall $PIM_OPT_LEVEL
	print_msg " > Resident code ..."
	${PIM_CROSS_COMPILE}gcc -c -march=armv7-a resident.c -o resident.o -mno-thumb-interwork -fno-stack-protector -fPIC -Wall $PIM_OPT_LEVEL -fno-toplevel-reorder
	${PIM_CROSS_COMPILE}gcc -c -march=armv7-a resident.c -o resident.s -mno-thumb-interwork -fno-stack-protector -fPIC -S -Wall $PIM_OPT_LEVEL -fno-toplevel-reorder
	print_msg " > Link ..."
	${PIM_CROSS_COMPILE}ld -T resident.ld resident.o boot.o -lc -lgcc -lgcc_eh -o resident.elf -non_shared -static $PIM_OPT_LEVEL \
		--library-path /usr/arm-linux-gnueabihf/lib/ \
		--library-path /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/
################################################################################################
elif [ $1 == "8" ]; then	# PIM is ARMv8
	print_msg " > Boot loader ..."
	${PIM_CROSS_COMPILE}gcc -march=armv8-a -nostdinc -o boot.o -c boot_arm8.S -Wall $PIM_OPT_LEVEL -DCNTFRQ=0x01800000 -DSYSREGS_BASE=0x1c010000 -DGIC_DIST_BASE=0x2c001000 -DGIC_CPU_BASE=0x2c002000 
	
	#-fno-builtin
	#-DUART_BASE=0x1c090000
	#-DPHYS_OFFSET=0x80000000
	
	print_msg " > Resident code ..."
	${PIM_CROSS_COMPILE}gcc -c -march=armv8-a resident.c -o resident.s -fno-stack-protector -S -Wall $PIM_OPT_LEVEL -Os -fno-toplevel-reorder
	${PIM_CROSS_COMPILE}gcc -c -march=armv8-a resident.c -o resident.o -fno-stack-protector -Wall $PIM_OPT_LEVEL    -Os -fno-toplevel-reorder
	
	#-fPIC
	#-fno-stack-protector
	
	print_msg " > Link ..."
	${PIM_CROSS_COMPILE}ld -T resident.ld resident.o boot.o -lc -lgcc -lgcc_eh -o resident.elf -non_shared -static $PIM_OPT_LEVEL \
		--library-path /usr/aarch64-linux-gnu/lib \
		--library-path /usr/lib/gcc-cross/aarch64-linux-gnu/4.8
fi
################################################################################################

if [ $BUILD_RESIDENT == TRUE ]; then					# Build the main resident code
	print_msg " > Resolve PIM_REG symbols ..."
	get_symbol_addr PIM_DEBUG_REG resident.elf
	get_symbol_addr PIM_STATUS_REG resident.elf
	get_symbol_addr PIM_COMMAND_REG resident.elf
	get_symbol_addr PIM_ERROR_REG resident.elf
	get_symbol_addr PIM_INTR_REG resident.elf
	get_symbol_addr PIM_VREG resident.elf
	get_symbol_addr PIM_SREG resident.elf
	get_symbol_addr PIM_SLICETABLE resident.elf
	get_symbol_addr PIM_SLICECOUNT resident.elf
	get_symbol_addr PIM_SLICEVSTART resident.elf
	get_symbol_addr PIM_M5_REG resident.elf
	get_symbol_addr PIM_M5_D1_REG resident.elf
	get_symbol_addr PIM_M5_D2_REG resident.elf
	get_symbol_addr PIM_DTLB_IDEAL_REFILL resident.elf
	get_symbol_addr PIM_DMA_MEM_ADDR resident.elf
	get_symbol_addr PIM_DMA_SPM_ADDR resident.elf
	get_symbol_addr PIM_DMA_NUMBYTES resident.elf
	get_symbol_addr PIM_DMA_COMMAND resident.elf
    get_symbol_addr PIM_DMA_STATUS resident.elf
    get_symbol_addr PIM_DMA_CLI resident.elf
    get_symbol_addr HMC_ATOMIC_INCR resident.elf
    get_symbol_addr HMC_ATOMIC_IMIN resident.elf
    get_symbol_addr HMC_ATOMIC_FADD resident.elf
    get_symbol_addr HMC_OPERAND resident.elf
	get_symbol_addr PIM_COPROCESSOR_CMD resident.elf

	pim_print_mem_map	# Print the memory map
	# Check if the size of these sections fit in available regions
	check_section_size $(echo $((16#${text_size:2})))   $(PIM_TEXT_SIZE) ".text"
	check_section_size $(echo $((16#${rodata_size:2}))) $(PIM_RODATA_SIZE) ".rodata"
	
	# Disassembly output (for debugging purposes)
	${PIM_CROSS_COMPILE}objdump -S --disassemble resident.elf > resident.elf.disasm
	
	if [ $OFFLOAD_THE_KERNEL == TRUE ]; then
		print_msg "Really offload the kernel (partial offloading)"
		cp resident.elf $M5_PATH/binaries/
	fi
	
################################################################################################
else													# Build the kernel to be offloaded
	print_msg " > Sanity checks ..."
	rm symbols_resident.elf.txt	# This is necessary

	# We want to make sure that across different compilations the symbol addresses have not changed.
	check_symbol_addr PIM_DEBUG_REG resident.elf
	check_symbol_addr PIM_STATUS_REG resident.elf
	check_symbol_addr PIM_COMMAND_REG resident.elf
	check_symbol_addr PIM_ERROR_REG resident.elf
	check_symbol_addr PIM_INTR_REG resident.elf
	check_symbol_addr PIM_VREG resident.elf
	check_symbol_addr PIM_SREG resident.elf
	check_symbol_addr PIM_SLICETABLE resident.elf
	check_symbol_addr PIM_SLICECOUNT resident.elf
	check_symbol_addr PIM_SLICEVSTART resident.elf
	check_symbol_addr PIM_M5_REG resident.elf
	check_symbol_addr PIM_M5_D1_REG resident.elf
	check_symbol_addr PIM_M5_D2_REG resident.elf
	check_symbol_addr PIM_DTLB_IDEAL_REFILL resident.elf
	check_symbol_addr PIM_DMA_MEM_ADDR resident.elf
	check_symbol_addr PIM_DMA_SPM_ADDR resident.elf
	check_symbol_addr PIM_DMA_NUMBYTES resident.elf
	check_symbol_addr PIM_DMA_COMMAND resident.elf	
	check_symbol_addr PIM_DMA_STATUS resident.elf
    check_symbol_addr PIM_DMA_CLI resident.elf
	check_symbol_addr HMC_ATOMIC_INCR resident.elf
    check_symbol_addr HMC_ATOMIC_IMIN resident.elf
    check_symbol_addr HMC_ATOMIC_FADD resident.elf
    check_symbol_addr HMC_OPERAND resident.elf
	check_symbol_addr PIM_COPROCESSOR_CMD resident.elf
	
	print_msg " > Dump the required section to $2.hex"
# 	get_symbol_addr execute_kernel resident.elf
# 	get_symbol_size execute_kernel resident.elf
	if [ $OFFLOAD_THE_KERNEL == TRUE ]; then
		hexdump_section resident.elf "text"   $2.hex
		hexdump_section resident.elf "rodata" $2.hex
	else
		touch $2.hex		# Because we are not actually offloading anything
	fi
	#hexdump_section resident.elf "symtab" $2.hex		TODO THIS IS NOT NECESSARY
	# Check if the size of these sections fit in available regions
	check_section_size $(echo $((16#${text_size:2})))   $(PIM_TEXT_SIZE) ".text"
	check_section_size $(echo $((16#${rodata_size:2}))) $(PIM_RODATA_SIZE) ".rodata"
	echo "  Done!"
	
	${PIM_CROSS_COMPILE}objdump -S --disassemble resident.elf > resident.elf.disasm.kernel
	
	if [ $OFFLOAD_THE_KERNEL == FALSE ]; then
		print_msg "Do not offload the computation kernel to PIM, just load it magically."
		cp resident.elf $M5_PATH/binaries/
	fi
fi
################################################################################################
