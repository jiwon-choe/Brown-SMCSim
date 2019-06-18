#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "params.h"

/****************************************************************************/
#define NC   ((unsigned long)(-1))

typedef struct Node
{
    /* Bellman-ford */
    unsigned long distance;
    unsigned long*  weights;            // List of weights (weighted graph)

    /* General parameters */
    unsigned long       ID;                 // ID of the current node
    unsigned long       out_degree;         // Number of successor nodes
    struct Node** successors;         // List of successors nodes
} node;

/****************************************************************************/

node* nodes;                 // Graph nodes
node**  successors_list;     // List of the successors (not used directly)

/****************************************************************************/

    unsigned long  weights_size;     // Size of the weights list
    unsigned long* weights_list;     // List of the weights (not used directly)

    void reset_graph_stats()
    {
        for ( unsigned long i=1; i< NODES; i++ )
        {
            nodes[i].distance = NC; // Infinity
        }
        nodes[0].distance = 0;       // node[0] is the starting node
    }

    unsigned long run_golden()
    {
        unsigned long checksum = 0;
        for ( unsigned r=0; r<NODES; r++ )
            for ( unsigned long c=0; c<nodes[r].out_degree; c++ ) // for node.successors
            {
                node*u = &nodes[r];
                node*v = nodes[r].successors[c];
                unsigned long w = nodes[r].weights[c];
                if ( u->distance != NC && v->distance > u->distance + w )
                    v->distance = u->distance + w;
                checksum += w;
            }
        return checksum;
    }

/****************************************************************************/
#include "create_topology.h"

void create_graph()
{
    create_topology();

    // Kernel specific initializations
    printf(" Initializing weights_list for the weighted graph ...\n");
    weights_size = 0;
    weights_list = NULL;

    for ( unsigned long i=0; i<NODES; i++ )
    {
        unsigned long d = nodes[i].out_degree;
        if ( d == 0 )
            nodes[i].weights = NULL;
        else
        {
            nodes[i].weights = (unsigned long*)malloc(d*sizeof(unsigned long));
            assert (nodes[i].weights);
            weights_size += d*sizeof(unsigned long);
        }
        for ( unsigned long j=0; j<d; j++)
            nodes[i].weights[j] = rand()%MAX_WEIGHT + 1;
    }
    
    weights_list = nodes[0].weights;
    reset_graph_stats();
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    printf("(main.cpp): [BF] Create the graph with %d nodes\n", NODES);
    create_graph();
    printf("(main.cpp): Running the golden model ... \n");
    unsigned long retval = run_golden();
    printf(" Golden model returned: %ld\n", retval);
    return 0;
}

