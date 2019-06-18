#!/bin/bash

echo -e "
#include \"defs.hh\"
#define $OFFLOADED_KERNEL_NAME
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define NODES $OFFLOADED_GRAPH_NODES
#define DENSITY $OFFLOADED_GRAPH_DENSITY
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define MAX_WEIGHT $OFFLOADED_GRAPH_MAX_WEIGHT
#define PAGERANK_MAX_ITERATIONS $PAGERANK_MAX_ITERATIONS
#define PAGERANK_MAX_ERROR $PAGERANK_MAX_ERROR
#define REQUIRED_MEM_SIZE (NODES*sizeof(node) + NODES*NODES*sizeof(ulong_t) )
$(set_if_true $DEBUG_PIM_APP "#define DEBUG_APP")
$(set_if_true $OFFLOAD_THE_KERNEL "#define OFFLOAD_THE_KERNEL")
" > _app_params.h

echo -e "
#define $OFFLOADED_KERNEL_NAME
" > kernel_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


