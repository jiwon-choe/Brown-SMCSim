#!/bin/bash

# print_msg "Loading Host Model: ARMv7 (Cortex-A15)"

export GEM5_NUMCPU=2
export HOST_CLOCK_FREQUENCY_GHz=2
export ARCH_ALIAS="arm7"
export GEM5_PLATFORM=ARM
export GEM5_MACHINETYPE="VExpress_EMM"
export GEM5_DTBFILE="$M5_PATH/binaries/vexpress.aarch32.ll_20131205.0-gem5.${GEM5_NUMCPU}cpu.dtb"
export GEM5_KERNEL="$M5_PATH/binaries/vmlinux.aarch32.ll_20131205.0-gem5"
export GEM5_DISKIMAGE="$M5_PATH/disks/linux-aarch32-ael.img"
export HOST_CROSS_COMPILE=arm-linux-gnueabihf-
export ARCH="arm"		# Notice this is case sensitive
export COMPILED_KERNEL_PATH=${SMC_WORK_DIR}/linux_kernel/linux-linaro-tracking-gem5-ll_20131205.0-gem5-a75e551/
export GEM5_CHECKPOINT_LOCATION="NONE"
export GEM5_CPUTYPE=timing

export L1I_CACHE_SIZE=32kB
export L1D_CACHE_SIZE=64kB
export L2_CACHE_SIZE=2MB

# This is used only for full-system simulation, but for comparison with modelsim we must use fcfs
export DRAM_SCHEDULING_POLICY_GEM5="frfcfs"
