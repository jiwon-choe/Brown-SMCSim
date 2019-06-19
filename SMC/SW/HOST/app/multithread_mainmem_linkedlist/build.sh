#!/bin/bash

if [ $OFFLOADED_KERNEL_NAME == sgraph_bfs ]; then step=7; else step=1; fi
echo -e "
#include \"defs.hh\"

#define $APP_TYPE
#define $OFFLOADED_KERNEL_NAME
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define REQUIRED_MEM_SIZE $REQUIRED_MEM_SIZE
 
#define KEY_LOWER_BOUND 1
#define KEY_UPPER_BOUND $KEY_UPPER_BOUND
#define INITIAL_LIST_SIZE $INITIAL_LIST_SIZE
#define MAX_LEVEL $MAX_LEVEL 
#define TOTAL_NUM_OPS $TOTAL_NUM_OPS
#define READ_ONLY_PERCENTAGE $READ_ONLY_PERCENTAGE
#define NUM_HOST_THREADS 8
$(set_if_true $DEBUG_ON "#define HOST_DEBUG_ON")
" > _app_params.h

echo -e "
#define $APP_TYPE
#define $OFFLOADED_KERNEL_NAME
" > kernel_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -std=c++11 -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


