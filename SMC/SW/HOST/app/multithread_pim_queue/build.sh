#!/bin/bash

echo -e "
#include \"defs.hh\"
#define $OFFLOADED_KERNEL_NAME
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define KEY_LOWER_BOUND 1
#define KEY_UPPER_BOUND $KEY_UPPER_BOUND
#define INITIAL_Q_SIZE $INITIAL_Q_SIZE
#define NUM_PARTITIONS $NUM_PARTITIONS
#define TOTAL_NUM_OPS $TOTAL_NUM_OPS
#define PUSH_PERCENTAGE $PUSH_PERCENTAGE
#define REQUIRED_MEM_SIZE 0
#define NUM_HOST_THREADS $NUM_HOST_THREADS
#define MAX_SEGMENT_BYTES $MAX_SEGMENT_BYTES
$(set_if_true $DEBUG_ON "#define HOST_DEBUG_ON")
" > _app_params.h

echo -e "
#define $OFFLOADED_KERNEL_NAME
" > kernel_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


