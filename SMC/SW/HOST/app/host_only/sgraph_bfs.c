#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "params.h"

/****************************************************************************/

typedef struct Node
{
    /* BFS */
    unsigned long distance;

    /* General parameters */
    unsigned long       ID;                 // ID of the current node
    unsigned long       out_degree;         // Number of successor nodes
    struct Node** successors;         // List of successors nodes
} node;

// queue operations (used for BFS algorithm)
#define queue_init     { head=0; tail=0; elements=0; }
#define queue_push(X)  { queue[head]=(unsigned long)X;  head++;  if (head>=NODES) head=0; elements++; }
#define queue_pop      { tail++; if (tail>=NODES) tail=0;  elements--; } 
#define queue_top      ( queue[tail] )
#define queue_empty    ( elements==0 )
#define queue_full     ( elements==(NODES-1) )

#define NC   ((unsigned long)(-1))

/****************************************************************************/

node* nodes;                 // Graph nodes
node**  successors_list;     // List of the successors (not used directly)

/****************************************************************************/
    unsigned long* queue;    // queue for BFS search
    unsigned long  head, tail, elements;

    void reset_graph_stats()
    {
        for ( unsigned long i=1; i< NODES; i++ )
        {
            nodes[i].distance = NC;
        }
        nodes[0].distance = 0;       // node[0] is the starting node
        queue_init;                  // Initialize queue
    }

    unsigned long run_golden()
    {
        // cout << "Visited 0 with distance 0" << endl;
        unsigned long total_distance = 0;

        for (int x=0; x<BFS_MAX_ITERATIONS; x++)
        {
            nodes[x].distance = 0;       // node[0] is the starting node
            queue_init;                  // Initialize queue
            queue_push(&nodes[x]);               // Push the first element
            while ( !(queue_empty) )
            {
                node* v = (node*)queue_top;
                queue_pop;
                for ( unsigned long c=0; c<v->out_degree ; c++ )
                {
                    node*succ = v->successors[c];
                    if (succ->distance == NC) // Infinite
                    {
                        succ->distance = v->distance + 1;
                        total_distance += succ->distance;
                        // cout << "Visited " << succ->ID << " with distance " << succ->distance << endl;
                        queue_push(succ);
                    }
                }
            }
        }
        return total_distance;
    }


/****************************************************************************/
#include "create_topology.h"

void create_graph()
{
    create_topology();

    // Kernel specific initialization
    queue = (unsigned long*)malloc(NODES*sizeof(unsigned long));

    reset_graph_stats();
}


/************************************************/
// Main
int main(int argc, char *argv[])
{
    printf("(main.cpp): [BFS] Create the graph with %d nodes\n", NODES);
    create_graph();
    printf("(main.cpp): Running the golden model ... \n");
    unsigned long retval = run_golden();
    printf(" Golden model returned: %ld\n", retval);
    return 0;
}

