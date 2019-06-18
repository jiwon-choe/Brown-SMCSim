#!/bin/bash

if [ $OFFLOADED_KERNEL_NAME == sgraph_bfs ]; then step=7; else step=1; fi
echo -e "
#include \"defs.hh\"
#define $OFFLOADED_KERNEL_NAME
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define NODES $OFFLOADED_GRAPH_NODES
#define MAX_OUTDEGREE $OFFLOADED_GRAPH_MAX_OUTDEGREE
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define PAGERANK_MAX_ITERATIONS $PAGERANK_MAX_ITERATIONS
#define BFS_MAX_ITERATIONS $BFS_MAX_ITERATIONS
#define PAGERANK_MAX_ERROR $PAGERANK_MAX_ERROR
#define step $step
#define REQUIRED_MEM_SIZE (NODES*sizeof(node) + NODES*MAX_OUTDEGREE*sizeof(node*) + NODES*MAX_OUTDEGREE*sizeof(ulong_t) $QUEUE_SIZE )
#define MAX_WEIGHT $OFFLOADED_GRAPH_MAX_WEIGHT
$(set_if_true $DEBUG_PIM_APP "#define DEBUG_APP")
$(set_if_true $OFFLOAD_THE_KERNEL "#define OFFLOAD_THE_KERNEL")
" > _app_params.h

echo -e "
#define $OFFLOADED_KERNEL_NAME
" > kernel_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


