#!/bin/bash

############################################################################################
# JIWON: used this to get ASPLOS'19 results for host FC, host lazy-lock linked-list
############################################################################################

GEM5_STATISTICS=(
"sim_ticks.pim"
"sim_ticks.host"
"sim_ticks.ratio_host_to_pim"
"power_related_to_host"
"power_related_to_pim"
)

LINKED_LIST_KERNEL_NAME=dummy_kernel

LINKED_LIST_TYPE=default 
HOST_APP_DIR=multithread_mainmem_linkedlist

# APP_TYPE: HOST_FC_SORT, HOST_LAZY_LOCK
export APP_TYPE=HOST_LAZY_LOCK
export PIM_BUILD="FALSE"
export USE_HOST_THREADS="TRUE"  # set as TRUE for fc_without_sort or fc_with_sort linked lists
export NUM_HOST_THREADS=8      # DON'T CHANGE THIS FIELD!! host app takes measurements for 2,4,6,8 threads all at once
export KEY_UPPER_BOUND=100000
export INITIAL_LIST_SIZE=80000
export MAX_LEVEL=16
export TOTAL_NUM_OPS=4200
export READ_ONLY_PERCENTAGE=90
export DEBUG_ON="FALSE"
export REQUIRED_MEM_SIZE=8388608 # 8MB (this field has no meaning anymore. originally used for PIM mainmem only, but PIM has new implementation now.) 


source UTILS/default_params.sh
create_scenario "$0/$*" "$HOST_APP_DIR-initialsize$INITIAL_LIST_SIZE-$TOTAL_NUM_OPS-$READ_ONLY_PERCENTAGE" "ARMv7 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm7.sh
load_model system/gem5_pim.sh
load_model gem5_perf_sim.sh				# Fast simulation without debugging

export DRAM_row_size=16  # JIWON: to keep memory size consistent with new PIM architecture
export OFFLOADED_KERNEL_NAME=$LINKED_LIST_KERNEL_NAME    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
export OFFLOADED_KERNEL_SUBNAME=$LINKED_LIST_TYPE
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
#cp $HOST_SW_DIR/app/app_utils.hh .  #added by jiwon 
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
