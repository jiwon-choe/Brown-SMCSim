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

# export GEM5_DEBUGFLAGS=Cache
# export PIM_DTLB_DUMP_ADDRESS=TRUE
#####################
#####################
export OFFLOADED_KERNEL_NAME=matrix_add	# Kernel name to offload (Look in SMC/SW/PIM/kernels)
export OFFLOADED_MATRIX_SIZE=200
#####################
#####################

source ./smc.sh -u $*	# Update these variables in the simulation environment
# load_model gem5_automated_sim.sh homo	# Automated simulation

####################################

print_msg "Build and copy the required files to the extra image ..."

#*******
clonedir $PIM_SW_DIR/resident
run ./build.sh $1	# Build the main resident code
run ./build.sh $1 "${OFFLOADED_KERNEL_NAME}" # Build a specific kernel code (name without suffix)
returntopwd
#*******
clonedir $HOST_SW_DIR/driver
cp ../resident/definitions.h .
run ./build.sh
returntopwd
#*******
clonedir $HOST_SW_DIR/api
cp ../resident/definitions.h .
cp ../driver/defs.h .
run ./build.sh
returntopwd
#*******
clonedir $HOST_SW_DIR/app/offload_matrix
cp ../api/pim_api.a ./libpimapi.a
cp ../api/*.hh .
cp $HOST_SW_DIR/app/app_utils.hh .
run ./build.sh
returntopwd
#*******

cd $SCENARIO_CASE_DIR

echo -e "
echo; echo \">>>> Install the driver\";
./ins.sh
echo; echo \">>>> Run the application and offload the kernel ...\";
./main
" > ./do
chmod +x ./do

copy_to_extra_image  driver/pim.ko driver/ins.sh ./do offload_matrix/main resident/${OFFLOADED_KERNEL_NAME}.hex 
returntopwd
 
####################################

source ./smc.sh $*
print_msg "Done!"

