#!/bin/bash

echo -e "
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define SIZE $OFFLOADED_MATRIX_SIZE
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define REQUIRED_MEM_SIZE ($OFFLOADED_MATRIX_SIZE*$OFFLOADED_MATRIX_SIZE*sizeof(ulong_t)*4)
$(set_if_true $DEBUG_PIM_APP "#define DEBUG_APP")
$(set_if_true $OFFLOAD_THE_KERNEL "#define OFFLOAD_THE_KERNEL")
" > _app_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


