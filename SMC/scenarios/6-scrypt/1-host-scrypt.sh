#!/bin/bash

GEM5_STATISTICS=(
"sim_ticks.pim"
"sim_ticks.host"
"sim_ticks.ratio_host_to_pim"
"power_related_to_host"
"power_related_to_pim"
)

PIM_KERNEL_NAME=dummy_kernel

KERNEL_TYPE=default 
HOST_APP_DIR=scrypt_host

export PASSPHRASE="pleaseletmein"
export SALT="SodiumChloride"
export KEY="7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2d5432955613f0fcf62d49705242a9af9e61e85dc0d651e40dfcf017b45575887"
export CPU_MEM_COST=16384 # N
export BLOCK_SIZE_PARAM=8 # r
export PARALLEL_PARAM=1 # p
export DK_LEN=64 # dkLen is the desired output key length 

export DEBUG_ON="FALSE"


source UTILS/default_params.sh
create_scenario "$0/$*" "$HOST_APP_DIR-$PASSPHRASE-$SALT-N$CPU_MEM_COST-r$BLOCK_SIZE_PARAM-p$PARALLEL_PARAM" "ARMv7 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm7.sh
load_model system/gem5_pim.sh
load_model gem5_perf_sim.sh				# Fast simulation without debugging

export DRAM_row_size=16  # JIWON: to keep memory size consistent with new PIM architecture
export OFFLOADED_KERNEL_NAME=$PIM_KERNEL_NAME    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
export OFFLOADED_KERNEL_SUBNAME=$KERNEL_TYPE
#####################
#####################
load_model common_params_date2016.sh 
#####################
#####################

source ./smc.sh -u $*	# Update these variables in the simulation environment
load_model gem5_automated_sim.sh homo		# Automated simulation

####################################

print_msg "Build and copy the required files to the extra image ..."

#*******
clonedir $PIM_SW_DIR/resident
cp $HOST_SW_DIR/app/$HOST_APP_DIR/defs.hh .
run ./build.sh  7   # Build the main resident code
run ./build.sh 7 "${OFFLOADED_KERNEL_NAME}" # Build a specific kernel code (name without suffix)
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
clonedir $HOST_SW_DIR/app/$HOST_APP_DIR
cp ../api/pim_api.a ./libpimapi.a
cp ../api/*.hh .
cp $HOST_SW_DIR/app/app_utils.hh .
run ./build.sh
returntopwd
#*******

cd $SCENARIO_CASE_DIR

echo -e "
echo; echo \">>>> Install the driver\";
./ins.sh $NUM_PIM_DEVICES
echo; echo \">>>> Run the application and offload the kernel ...\";
./main
" > ./do
chmod +x ./do

copy_to_extra_image  driver/pim.ko driver/ins.sh ./do $HOST_APP_DIR/main resident/${OFFLOADED_KERNEL_NAME}.hex 
returntopwd

####################################

source ./smc.sh $*
	
finalize_gem5_simulation
plot_bar_chart "sim_ticks.pim" 0 "(ps)" #--no-output
plot_bar_chart "sim_ticks.host" 0 "(ps)" #--no-output
print_msg "Done!"
