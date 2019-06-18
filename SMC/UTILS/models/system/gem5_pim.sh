#!/bin/bash
# Parameters for the single processor PIM model
export GEM5_PIM_KERNEL=resident.elf
export PIM_CLOCK_FREQUENCY_GHz="2.0"		# Clock frequency of the PIM processor (GHz)
export PIM_SPM_ACCESSTIME_ns="0.3"			# This is a very fast SPM accessible by PIM in a single cycle
export HAVE_PIM_DEVICE="TRUE"
export PIM_ADDRESS_BASE="0x70000000"
export PIM_ADDRESS_SIZE="1MB"
export PIM_VREG_SIZE=4096
export PIM_SREG_COUNT=8
export PIM_DTLB_SIZE=16
export PIM_DMA_XFER_SIZE=256                # Software parameter limiting the transfer size when possible
export PIM_DMA_MAX_XFER_SIZE=512            # This limits the size of the memory allocated in the SPM for each DMA transfer
export USE_HMC_ATOMIC_CMD=TRUE              # Use atomic hmc commands if possible (depends on the kernel code)
export PIM_DTLB_DO_IDEAL_REFILL=TRUE
export PIM_MERGE_SLICES=TRUE                # Merge physically and virtually contiguous slices into bigger ones

if [ -z $HOST_CROSS_COMPILE ]; then
	print_err "HOST_CROSS_COMPILE is not set!"
fi
export PIM_CROSS_COMPILE=$HOST_CROSS_COMPILE

export HAVE_PIM_CLUSTER="FALSE"

# Modify these based on the size of each section
# Notice: if you reorder these sections, you have to 
#  modify the formulats for PIM_XXX_SIZE
export PIM_PLT_OFFSET="0x00000600"
export PIM_TEXT_OFFSET="0x00000700"
export PIM_RODATA_OFFSET="0x00003000"
export PIM_ARRAY_OFFSET="0x00004000"
export PIM_GOT_OFFSET="0x00005000"
export PIM_DATA_OFFSET="0x00006000"
export PIM_BSS_OFFSET="0x00007000"
export PIM_STACK_OFFSET="0x0000A000"	# Offset of the PIM's stack

# Host's memory from PIM's point of view (This must be after PIM's SPM)
export PIM_HOST_OFFSET=0x$(printf "%08X" $(($(conv_to_bytes $PIM_ADDRESS_SIZE))));

################################################
###################UTILITIES####################
################################################

# Size of the .text section
function PIM_TEXT_SIZE()
{
	printf $(($PIM_RODATA_OFFSET - $PIM_TEXT_OFFSET));
}

# Size of the .rodata section
function PIM_RODATA_SIZE()
{
	printf $(($PIM_ARRAY_OFFSET - $PIM_RODATA_OFFSET));
}

# Print PIM's memory map
function pim_print_mem_map()
{
    PIM_PLT_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_PLT_OFFSET})))
    PIM_TEXT_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_TEXT_OFFSET})))
	PIM_RODATA_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_RODATA_OFFSET})))
	PIM_ARRAY_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_ARRAY_OFFSET})))
	PIM_GOT_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_GOT_OFFSET})))
	PIM_DATA_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_DATA_OFFSET})))
	PIM_BSS_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_BSS_OFFSET})))
	PIM_STACK_PHYS=0x$(printf "%X" $((${PIM_ADDRESS_BASE} + ${PIM_STACK_OFFSET})))
	PIM_MAX_OFFSET=0x$(printf "%08X" $(($(conv_to_bytes $PIM_ADDRESS_SIZE)-1)));
	PIM_MAX_PHYS=0x$(printf "%08X" $((${PIM_ADDRESS_BASE} + ${PIM_MAX_OFFSET})))
	
	echo -e " $(extend "" 75 "_")"
	echo -e "|$(extend "[PIM Virtual Address]" 25 "_")$(extend "[System Physical Addr]" 25 "_")$(extend "[Meaning]" 25 "_")|"
	echo -e "|$(extend "0x00000000" 25 "_")$(extend "$PIM_ADDRESS_BASE" 25 "_")$(extend "SPM Start (bootldr)" 25 "_")|"
    echo -e "|$(extend "$PIM_PLT_OFFSET" 25 "_")$(extend "$PIM_PLT_PHYS" 25 "_")$(extend ".text" 25 "_")|"
    echo -e "|$(extend "$PIM_TEXT_OFFSET" 25 "_")$(extend "$PIM_TEXT_PHYS" 25 "_")$(extend ".text" 25 "_")|"
	echo -e "|$(extend "$PIM_RODATA_OFFSET" 25 "_")$(extend "$PIM_RODATA_PHYS" 25 "_")$(extend ".rodata" 25 "_")|"
	echo -e "|$(extend "$PIM_ARRAY_OFFSET" 25 "_")$(extend "$PIM_ARRAY_PHYS" 25 "_")$(extend ".array" 25 "_")|"
	echo -e "|$(extend "$PIM_GOT_OFFSET" 25 "_")$(extend "$PIM_GOT_PHYS" 25 "_")$(extend ".got" 25 "_")|"
	echo -e "|$(extend "$PIM_DATA_OFFSET" 25 "_")$(extend "$PIM_DATA_PHYS" 25 "_")$(extend ".data" 25 "_")|"
	echo -e "|$(extend "$PIM_BSS_OFFSET" 25 "_")$(extend "$PIM_BSS_PHYS" 25 "_")$(extend ".bss" 25 "_")|"
	echo -e "|$(extend "$PIM_STACK_OFFSET" 25 "_")$(extend "$PIM_STACK_PHYS" 25 "_")$(extend ".stack (dir:bss)" 25 "_")|"
	echo -e "|$(extend "$PIM_MAX_OFFSET" 25 "_")$(extend "$PIM_MAX_PHYS" 25 "_")$(extend "SPM End" 25 "_")|"
	
	
	echo -e "|$(extend "$PIM_HOST_OFFSET" 25 "_")$(extend "0x80000000" 25 "_")$(extend "Main System DRAM" 25 "_")|"
	echo -e " $(extend "" 75 "_")|"
}