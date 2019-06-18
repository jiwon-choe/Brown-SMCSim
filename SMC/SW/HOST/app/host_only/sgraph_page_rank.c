#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "params.h"

/****************************************************************************/

typedef struct Node
{
    float page_rank;
    float next_rank;

    /* General parameters */
    unsigned long       ID;                 // ID of the current node
    unsigned long       out_degree;         // Number of successor nodes
    struct Node** successors;         // List of successors nodes
} node;
/****************************************************************************/

node* nodes;                 // Graph nodes
node**  successors_list;     // List of the successors (not used directly)

/****************************************************************************/

    void reset_graph_stats()
    {
        for ( unsigned long i=0; i< NODES; i++ )
        {
            nodes[i].page_rank = 0;
            nodes[i].next_rank = 0;
        }
    }

    unsigned long run_golden()
    {
        for ( unsigned long i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / NODES;
            nodes[i].next_rank = 0.15 / NODES;
        }
        unsigned long count = 0;
        float diff = 0.0;
        do {
            for ( unsigned long i=0; i<NODES; i++ )
            {
                float delta = 0.85 * nodes[i].page_rank / nodes[i].out_degree;
                for ( unsigned long j=0; j<nodes[i].out_degree; j++ ) // for node.successors
                        nodes[i].successors[j]->next_rank += delta;
            }
            diff = 0.0;
            for ( unsigned long i=0; i<NODES; i++ )
            {
                diff += fabsf(nodes[i].next_rank - nodes[i].page_rank);
                nodes[i].page_rank = nodes[i].next_rank;
                nodes[i].next_rank = 0.15 / NODES;
            }
            printf( "ITERATION: %ld   ERROR: %f\n", count, diff);
            //print_graph();
        } while (++count < PAGERANK_MAX_ITERATIONS && diff > PAGERANK_MAX_ERROR);

        return (unsigned long)(diff * 1000000.0); // Convert error to fixed point integer
    }

/****************************************************************************/
#include "create_topology.h"

void create_graph()
{
    create_topology();

    reset_graph_stats();
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    printf("(main.cpp): [PR] Create the graph with %d nodes\n", NODES);
    create_graph();
    printf("(main.cpp): Running the golden model ... \n");
    unsigned long retval = run_golden();
    printf(" Golden model returned: %ld\n", retval);
    return 0;
}

