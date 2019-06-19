#!/bin/bash

GEM5_STATISTICS=(
"sim_ticks.pim"
"sim_ticks.host"
"sim_ticks.ratio_host_to_pim"
"power_related_to_host"
"power_related_to_pim"
)

# singlepim_queue+main_singlepim or clean_queue+main
# (in order to run single PIM queue, use main_singlepim.cc in SW/HOST/multithread_pim_queue as the main file.)

QUEUE_KERNEL_NAME=clean_queue #use singlepim_queue OR clean_queue
QUEUE_TYPE=default 
HOST_APP_DIR=multithread_pim_queue

export USE_HOST_THREADS="TRUE"  
export NUM_HOST_THREADS=8
# we want to make a 4MB initial size queue on host (to exceed L2 cache size).
# each host node is 8 bytes. each PIM node is 4 bytes. 
# (524288 * 4bytes/node / 2partitions = 1048576 bytes/partition)
# (524288 * 4bytes/node / 4partitions = 524288 bytes/partition)
export MAX_SEGMENT_BYTES=1048576
export KEY_UPPER_BOUND=1000 # variable
# (match host queue)
export INITIAL_Q_SIZE=524288 
export NUM_PARTITIONS=8
export TOTAL_NUM_OPS=786432 # variable by user
export PUSH_PERCENTAGE=50 # variable by user
export DEBUG_ON="FALSE"


source UTILS/default_params.sh
create_scenario "$0/$*" "$MAX_SEGMENT_BYTES-PIMCOREALWAYSON-$PIM_CORE_ALWAYS_ON" "ARMv7 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm7.sh
load_model system/gem5_new_pim.sh
load_model gem5_perf_sim.sh				# Fast simulation without debugging

export DRAMCTRL_EXTRA_ROWBUFFER_SIZE=256
export GEM5_PIM_MEMTYPE=BufferHMCVault # HMCVault (close page), OpenPageHMCVault, BufferHMCVault
export OFFLOADED_KERNEL_NAME=$QUEUE_KERNEL_NAME    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
export OFFLOADED_KERNEL_SUBNAME=$QUEUE_TYPE
#####################
#####################
#load_model common_params_date2016.sh 
export PIM_OPT_LEVEL=-O0    # copied only necessary params from common_params_date2016.sh
export HOST_OPT_LEVEL=-O0   # copied only necessary params from common_params_date2016.sh
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
