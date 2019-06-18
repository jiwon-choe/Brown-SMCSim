#!/bin/bash

# print_msg "Loading Host Model: ARMv8 (Cortex-A57)"

export GEM5_NUMCPU=2
export HOST_CLOCK_FREQUENCY_GHz=2
export ARCH_ALIAS="arm8"
export GEM5_PLATFORM=ARM
export GEM5_MACHINETYPE="VExpress_EMM64"
export GEM5_DTBFILE="$M5_PATH/binaries/vexpress.aarch64.20140821.dtb"
export GEM5_KERNEL="$M5_PATH/binaries/vmlinux.aarch64.20140821"
export GEM5_DISKIMAGE="$M5_PATH/disks/linaro-minimal-aarch64.img"
export HOST_CROSS_COMPILE=aarch64-linux-gnu-
export ARCH="arm64"		# Notice this is case sensitive
export COMPILED_KERNEL_PATH=${SMC_WORK_DIR}/linux_kernel/linux-aarch64-gem5-20140821/	# Compiled linux kernel
export GEM5_CHECKPOINT_LOCATION="NONE"		# Full address of the checkpoint to restore
export GEM5_CPUTYPE=timing

export L1I_CACHE_SIZE=32kB
export L1D_CACHE_SIZE=64kB
export L2_CACHE_SIZE=4MB

# This is used only for full-system simulation, but for comparison with modelsim we must use fcfs
export DRAM_SCHEDULING_POLICY_GEM5="frfcfs"
