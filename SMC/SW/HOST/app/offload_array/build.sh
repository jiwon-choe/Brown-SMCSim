#!/bin/bash

echo -e "
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define NUM_TURNS $OFFLOADED_NUM_TURNS
#define ARRAY_SIZE $OFFLOADED_ARRAY_SIZE
#define WALK_STEP	$OFFLOADED_WALK_STEP
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define REQUIRED_MEM_SIZE ($OFFLOADED_ARRAY_SIZE*sizeof(ulong_t))
$(set_if_true $INITIALIZE_ARRAY "#define INITIALIZE_ARRAY")
$(set_if_true $DEBUG_PIM_APP "#define DEBUG_APP")
$(set_if_true $OFFLOAD_THE_KERNEL "#define OFFLOAD_THE_KERNEL")
" > _app_params.h


print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


