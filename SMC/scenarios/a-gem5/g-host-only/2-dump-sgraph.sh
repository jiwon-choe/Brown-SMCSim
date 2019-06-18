#!/bin/bash
VALUES0=( sgraph_page_rank ) # sgraph_bfs sgraph_page_rank sgraph_teenage_follower sgraph_bellman_ford )

# This scenario runs the graph application on the real HOST machine (not in the simulated HOST/PIM systems in gem5)

for V0 in ${VALUES0[*]}
do
	source UTILS/default_params.sh
	create_scenario "$0/$*" "$V0" "ARMv$1 + HMC2011 + Linux (VExpress_EMM) + PIM(ARMv7)"

    export OFFLOADED_KERNEL_NAME=$V0    # Kernel name to offload (Look in SMC/SW/PIM/kernels)
	
	#*******
	clonedir $HOST_SW_DIR/app/host_only
	run ./build-x64.sh
	
	./main
	
	returntopwd
	#*******
	
done

print_msg "Done!"
