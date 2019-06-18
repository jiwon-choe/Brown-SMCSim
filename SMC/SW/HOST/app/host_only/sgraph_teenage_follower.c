#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "params.h"


/****************************************************************************/

typedef struct Node
{
    /* Average Teenage Follower */
    char teenager;
    unsigned long followers;

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
        nodes[i].followers = 0;
    }
}

unsigned long run_golden()
{
    for ( unsigned long r=0; r<NODES; r++ )
    {
        if ( nodes[r].teenager )
            for ( unsigned long c=0; c<nodes[r].out_degree ; c++ )
            {
                node*succ = nodes[r].successors[c];
                succ->followers++;
            }
    }
    unsigned long total_followers = 0;
    for ( unsigned long r=0; r<NODES; r++ )
        total_followers += nodes[r].followers;
    return total_followers;
}

/****************************************************************************/
#include "create_topology.h"

void create_graph()
{
    create_topology();

    // Kernel specific initializations
    for ( unsigned long i=0; i<NODES; i++ )
    {
        nodes[i].teenager = (rand()%2);
    }

    reset_graph_stats();
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    printf("(main.cpp): [ATF] Create the graph with %d nodes\n", NODES);
    create_graph();
    printf("(main.cpp): Running the golden model ... \n");
    unsigned long retval = run_golden();
    printf(" Golden model returned: %ld\n", retval);
    return 0;
}

