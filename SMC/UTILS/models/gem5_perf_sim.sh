#!/bin/bash
# Fast simulation inside gem5
export GEM5_SIM_MODE=opt # fast		# opt or fast

export DEBUG_PIM_RESIDENT=FALSE
export DEBUG_PIM_DRIVER=FALSE
export DEBUG_PIM_API=FALSE
export DEBUG_PIM_APP=FALSE
export GEM5_VERBOSITY="--quiet"			# {--verbose, --quiet}

export PIM_OPT_LEVEL="-O0"
export HOST_OPT_LEVEL="-O0"

export PIM_FIRMWARE_CHECKS=FALSE      # Sanity checks on the firmware size and addresses

if [ $GEM5_ENABLE_COMM_MONITORS == TRUE ]; then
    print_war "Please disable GEM5_ENABLE_COMM_MONITORS to further increase the simulation speed."
fi

export GATHER_CLUSTER_STATS=TRUE

