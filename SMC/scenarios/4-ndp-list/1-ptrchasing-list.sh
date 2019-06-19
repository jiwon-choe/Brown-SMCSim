#!/bin/bash

############################################################################################
# JIWON: used this to get ASPLOS'19 results for NDP linked-list, NDP skiplist
############################################################################################

GEM5_STATISTICS=(
"sim_ticks.pim"
"sim_ticks.host"
"sim_ticks.ratio_host_to_pim"
"power_related_to_host"
"power_related_to_pim"
)

# clean_linked_list, clean_skiplist
LIST_KERNEL_NAME=clean_linked_list
# fc_with_sort_nonstop (linked list), default (skiplist) 
LIST_TYPE=fc_with_sort_nonstop
# multithread_nonstop_pim_linkedlist, multithread_nonstop_pim_skiplist
HOST_APP_DIR=multithread_nonstop_pim_linkedlist

export USE_HOST_THREADS="TRUE"
export NUM_HOST_THREADS=8
export KEY_UPPER_BOUND=30
export INITIAL_LIST_SIZE=20
export NUM_PARTITIONS=1
export TOTAL_NUM_OPS=40
export READ_ONLY_PERCENTAGE=90
export DEBUG_ON="TRUE"


source UTILS/default_params.sh
create_scenario "$0/$*" "HOST-$HOST_APP_DIR-PIM-$LIST_KERNEL_NAME-threads$NUM_HOST_THREADS" "ARMv7 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm7.sh
load_model system/gem5_new_pim.sh
#load_model gem5_perf_sim.sh				# Fast simulation without debugging

#export DRAMCTRL_EXTRA_ROWBUFFER_SIZE=8 # 8 (bytes) for linked list, 128 (bytes) for skiplist
export GEM5_PIM_MEMTYPE=OpenPageHMCVault # HMCVault (close page), OpenPageHMCVault, BufferHMCVault
export OFFLOADED_KERNEL_NAME=$LIST_KERNEL_NAME    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
export OFFLOADED_KERNEL_SUBNAME=$LIST_TYPE
#####################
#load_model common_params_date2016.sh 
export PIM_OPT_LEVEL=-O0    # copied only necessary params from common_params_date2016.sh
export HOST_OPT_LEVEL=-O0   # copied only necessary params from common_params_date2016.sh
#####################

source ./smc.sh -u $*	# Update these variables in the simulation environment
load_model gem5_automated_sim.sh homo		# Automated simulation

####################################

print_msg "Build and copy the required files to the extra image ..."

#*******
clonedir $PIM_SW_DIR/resident
cp $HOST_SW_DIR/app/$HOST_APP_DIR/defs.hh .
cp $HOST_SW_DIR/app/app_utils.hh .  #added by jiwon 
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
#echo -e "
#echo; echo \">>>> Run the application and offload the kernel ...\";
#./main
#" > ./do
chmod +x ./do

copy_to_extra_image  driver/pim.ko driver/ins.sh ./do $HOST_APP_DIR/main resident/${OFFLOADED_KERNEL_NAME}.hex 
returntopwd

####################################

source ./smc.sh $*
	
finalize_gem5_simulation
plot_bar_chart "sim_ticks.pim" 0 "(ps)" #--no-output
plot_bar_chart "sim_ticks.host" 0 "(ps)" #--no-output
print_msg "Done!"
