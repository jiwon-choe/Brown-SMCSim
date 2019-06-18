#!/bin/bash
if ! [ -z $1 ] && ( [ $1 == "7" ] || [ $1 == "8" ] ); then echo "Arch: ARM$1"; else echo "First argument must be architecture: 7 or 8 (ARM7 or ARM8)"; exit; fi

GEM5_STATISTICS=(
"sim_ticks.host"
)

VALUES0=( sgraph_page_rank ) # sgraph_bfs sgraph_page_rank sgraph_teenage_follower sgraph_bellman_ford )

for V0 in ${VALUES0[*]}
do
	source UTILS/default_params.sh
	create_scenario "$0/$*" "$V0" "ARMv$1 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"
	
	####################################
	load_model memory/hmc_2011.sh
	load_model system/gem5_fullsystem_arm$1.sh
	load_model gem5_perf_sim.sh				# Fast simulation without debugging
	
    export OFFLOADED_KERNEL_NAME=$V0    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
    
	source ./smc.sh -u $*	# Update these variables in the simulation environment
	load_model gem5_automated_sim.sh homo		# Automated simulation
	
	####################################

	print_msg "Build and copy the required files to the extra image ..."

	#*******
	clonedir $HOST_SW_DIR/app/host_only
	run ./build.sh
	returntopwd
	#*******

	cd $SCENARIO_CASE_DIR

	echo -e "
	./main
	" > ./do
	chmod +x ./do

	copy_to_extra_image  ./do host_only/main
	returntopwd
	
	####################################

	source ./smc.sh $*
done
	
#finalize_gem5_simulation
print_msg "Done!"
