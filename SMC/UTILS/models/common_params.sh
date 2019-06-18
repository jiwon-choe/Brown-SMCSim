
export OFFLOADED_GRAPH_NODES=100000          # Number of nodes
export OFFLOADED_GRAPH_MAX_OUTDEGREE=50     # Maximum outdegree (sparse graph)
export PAGERANK_MAX_ITERATIONS=100
export PAGERANK_MAX_ERROR=0.05
export OFFLOADED_GRAPH_MAX_WEIGHT=10        # Maximum weight on the edges (only for bellman-ford)
export QUEUE_SIZE="+(NODES*sizeof(ulong_t))"
export BFS_MAX_ITERATIONS=10
# Matrix Addition
export OFFLOADED_MATRIX_SIZE=1024

# To estimate power consumption, we must reduce the total memory (as explained in the paper)
# export N_MEM_DIES=1

export PIM_OPT_LEVEL=-O0
export HOST_OPT_LEVEL=-O0
