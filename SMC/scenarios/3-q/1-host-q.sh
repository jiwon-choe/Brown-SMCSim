#!/bin/bash
GEM5_STATISTICS=(
"sim_ticks.pim"
"sim_ticks.host"
"sim_ticks.ratio_host_to_pim"
"power_related_to_host"
"power_related_to_pim"
)


QUEUE_KERNEL_NAME=dummy_kernel
HOST_APP_DIR=multithread_fc_q # choose among: multithread_fc_q, multithread_lockfree_queue

export QUEUE_TYPE=FLATCOMBINE # choose among: FLATCOMBINE, LOCKFREE
export NUM_HOST_THREADS=8 # 1, 3, 7 depending number of cores (instead of 2, 4, 8, since PIM can only utilize NUM_CORES-1 threads for list ops)
export KEY_UPPER_BOUND=1000 # variable depending on available memory size
#export INITIAL_Q_SIZE=2048 #524288 # variable by user (to make initial size of queue 4MB: 4*20^20 bytes / 8 bytes/node = 2^19)
export INITIAL_Q_SIZE=524288 # JIWON: for the modified host queue (circular buffer instead of linked list), this would be 2MB
export TOTAL_NUM_OPS=786432 # variable by user (2^19/2 * 3, in order to make deqs dequeue from newly enqed as well)
export PUSH_PERCENTAGE=50 # variable by user
export DEBUG_ON="FALSE" # TRUE or FALSE, TRUE sets HOST_DEBUG_ON and PIM_DEBUG_ON
export DOUBLE_FC="TRUE" 

source UTILS/default_params.sh
create_scenario "$0/$*" "$HOST_APP_DIR-partition$NUM_PARTITIONS-threads$NUM_HOST_THREADS-initialsize$INITIAL_LIST_SIZE-$(zpad $TOTAL_NUM_OPS 6)-$(zpad $PUSH_PERCENTAGE 3)" "ARMv7 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm7.sh
load_model system/gem5_pim.sh
load_model gem5_perf_sim.sh				# Fast simulation without debugging

export DRAM_row_size=16  # JIWON: to keep memory size consistent with new PIM architecture
export OFFLOADED_KERNEL_NAME=$QUEUE_KERNEL_NAME    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
#####################
#####################
load_model common_params_date2016.sh #TODO: jiwon -- possibly remove? contains mostly unnecessary graph parameters for sgraph scenarios
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


