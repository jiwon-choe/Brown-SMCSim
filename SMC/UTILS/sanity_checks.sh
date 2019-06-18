#!/bin/bash

if ! [ -z $LIST_PREVIOUS_RESULTS ]; then return; fi

if [ $OFFLOAD_THE_KERNEL == FALSE ]; then
    print_war "OFFLOAD_THE_KERNEL=FALSE, be careful that the kernel won't be actually offloaded! So you have to delete the checkpoint everytime"
fi

if [ -z $__UPDATE_ONLY ] || [ $__UPDATE_ONLY == FALSE ]; then
	if [ $GEM5_AUTOMATED_SIMULATION == TRUE ] && [ $OFFLOAD_THE_KERNEL == FALSE ] && [ $GEM5_AUTOMATED_SIMULATION_MODE == homo ]; then
		print_err "OFFLOAD_THE_KERNEL=FALSE. This means that the computation kernel will be magically loaded at the start of the simulation. So a checkpointed simulation cannot offload another kernel! This means that now the computation kernel is a part of the hardware and a change in it requires a new checkpoint to be taken."
		exit
	fi
fi

if [ $HOST_BURST_SIZE_B -gt $SMC_BURST_SIZE_B ]; then
	print_err "HOST_BURST_SIZE_B must be less than or equal to SMC_BURST_SIZE_B"
	exit
elif [ $HOST_BURST_SIZE_B -lt $SMC_BURST_SIZE_B ]; then
	print_war "HOST_BURST_SIZE_B < SMC_BURST_SIZE_B, SMC burst capability is not fully utilized!"
fi

if [ $HAVE_PIM_CLUSTER == TRUE ] && [ $HAVE_PIM_DEVICE == TRUE ]; then
    print_err "These two options cannot coexist! HAVE_PIM_CLUSTER and HAVE_PIM_DEVICE"
fi