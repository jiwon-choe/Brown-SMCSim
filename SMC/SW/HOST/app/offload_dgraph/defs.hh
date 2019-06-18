#ifndef DEFS_H_GRAPH
#define DEFS_H_GRAPH

#include "kernel_params.h"


typedef struct Node
{
    
    /* Average Teenage Follower */
    bool teenager;
    ulong_t followers;

    /* PageRank */
    float page_rank;
    float next_rank;

    /* Bellman-Ford */
    ulong_t distance;

    /* General parameters */
    ulong_t out_degree;         // Number of successor nodes
} node;

#define NC   ((ulong_t)(-1))

#endif