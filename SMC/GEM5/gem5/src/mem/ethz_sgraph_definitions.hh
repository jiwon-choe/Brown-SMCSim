#ifndef DEFS_H_GRAPH
#define DEFS_H_GRAPH

/*
Warning: this file should be updated based on ./SW/HOST/app/offload_sgraph/defs.hh
Notice: you cannot use pointer types, because they are architecture dependent
*/

/* Average Teenage Follower */
typedef struct Node_ATF_32
{
    bool           teenager;
    uint32_t       followers;
    /* General parameters */
    uint32_t       ID;                 // ID of the current node
    uint32_t       out_degree;         // Number of successor nodes
    uint32_t       successors;         // List of successors nodes
} node_atf_32;

/* PageRank */
typedef struct Node_PR_32
{
    float page_rank;
    float next_rank;
    /* General parameters */
    uint32_t       ID;                 // ID of the current node
    uint32_t       out_degree;         // Number of successor nodes
    uint32_t       successors;         // List of successors nodes
} node_pr_32;

/* BFS */
typedef struct Node_BFS_32
{
    uint32_t       distance;
    /* General parameters */
    uint32_t       ID;                 // ID of the current node
    uint32_t       out_degree;         // Number of successor nodes
    uint32_t       successors;         // List of successors nodes
} node_bfs_32;

/* Bellman-ford */
typedef struct Node_BF_32
{
    uint32_t       distance;
    uint32_t       weights;            // List of weights (weighted graph)
    /* General parameters */
    uint32_t       ID;                 // ID of the current node
    uint32_t       out_degree;         // Number of successor nodes
    uint32_t       successors;         // List of successors nodes
} node_bf_32;

/* Average Teenage Follower */
typedef struct Node_ATF_64
{
    bool           teenager;
    uint64_t       followers;
    /* General parameters */
    uint64_t       ID;                 // ID of the current node
    uint64_t       out_degree;         // Number of successor nodes
    uint64_t       successors;         // List of successors nodes
} node_atf_64;

/* PageRank */
typedef struct Node_PR_64
{
    float page_rank;
    float next_rank;
    /* General parameters */
    uint64_t       ID;                 // ID of the current node
    uint64_t       out_degree;         // Number of successor nodes
    uint64_t       successors;         // List of successors nodes
} node_pr_64;

/* BFS */
typedef struct Node_BFS_64
{
    uint64_t       distance;
    /* General parameters */
    uint64_t       ID;                 // ID of the current node
    uint64_t       out_degree;         // Number of successor nodes
    uint64_t       successors;         // List of successors nodes
} node_bfs_64;

/* Bellman-ford */
typedef struct Node_BF_64
{
    uint64_t       distance;
    uint64_t       weights;            // List of weights (weighted graph)
    /* General parameters */
    uint64_t       ID;                 // ID of the current node
    uint64_t       out_degree;         // Number of successor nodes
    uint64_t       successors;         // List of successors nodes
} node_bf_64;

// // queue operations (used for BFS algorithm)
// #define queue_init     { head=0; tail=0; elements=0; }
// #define queue_push(X)  { queue[head]=X;  head++;  if (head>=NODES) head=0; elements++; }
// #define queue_pop      { tail++; if (tail>=NODES) tail=0;  elements--; } 
// #define queue_top      ( queue[tail] )
// #define queue_empty    ( elements==0 )
// #define queue_full     ( elements==(NODES-1) )

// #define NC   ((uint32_t)(-1))

#endif