#!/bin/bash
if ! [ -z $1 ] && ( [ $1 == "7" ] || [ $1 == "8" ] ); then echo "Arch: ARM$1"; else echo "First argument must be architecture: 7 or 8 (ARM7 or ARM8)"; exit; fi

source UTILS/default_params.sh
create_scenario "$0/$*" "default" "ARMv$1 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"
####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm$1.sh
load_model system/gem5_pim.sh

# Reduce DRAM size to speedup simulation
export N_INIT_PORT=2
export DRAM_row_size="17"
export HAVE_L1_CACHES="TRUE"
export HAVE_L2_CACHES="TRUE"

# # export GEM5_DEBUGFLAGS=ethz_TLB
# export GEM5_CHECKPOINT_RESTORE="TRUE"
# export GEM5_CHECKPOINT_LOCATION="/home/erfan/projects/HUGE_FILES/SMCV3.OUT/checkpoints/cpt.39176064883500"

source ./smc.sh -u $*	# Update these variables in the simulation environment

if [ $1 == "8" ]; then
	print_msg "Temporarily, it is not possible to mix ARMv8 host with ARMv7 PIM. In the next version, we will support this feature."
	exit
fi

####################################

print_msg "Building and copying required files to the extra image ..."

clonedir $HOST_SW_DIR/driver b0
run ./build.sh
returntopwd

clonedir $HOST_SW_DIR/tests/testdriver b1
run ./build.sh
returntopwd

clonedir $HOST_SW_DIR/tests/helloarm b2
run ./build.sh
returntopwd

clonedir $HOST_SW_DIR/tests/testphysmem b3
source ./build-arm.sh
returntopwd

cd $SCENARIO_CASE_DIR
copy_to_extra_image b1/testdriver b2/helloarm b3/devmem2 b0/pim.ko b0/ins.sh 
returntopwd

clonedir $PIM_SW_DIR/tests/hellopim b4
run ./build.sh
cp test.elf $M5_PATH/binaries/resident.elf
returntopwd

####################################

source ./smc.sh $*
print_msg "Done!"

