#ifndef UTILS_H_SGRAPH
#define UTILS_H_SGRAPH

#include "_app_params.h"
#include "defs.hh"
#include <math.h>

node* nodes;                 // Graph nodes
ulong_t successors_size;     // Size of the successors list
node**  successors_list;     // List of the successors (not used directly)

//##############################################################################
//##############################################################################
#ifdef sgraph_teenage_follower

    void reset_graph_stats()
    {
        for ( ulong_t i=0; i< NODES; i++ )
        {
            nodes[i].followers = 0;
        }
    }

    void print_graph()
    {
        for ( ulong_t i=0; i<NODES; i++ )
            cout << "Node: " << i << " Teenager:" << nodes[i].teenager << " Followers: " << nodes[i].followers << endl;
    }

    ulong_t run_golden()
    {
        for ( ulong_t r=0; r<NODES; r++ )
        {
            if ( nodes[r].teenager )
                for ( ulong_t c=0; c<nodes[r].out_degree ; c++ )
                {
                    node*succ = nodes[r].successors[c];
                    succ->followers++;
                }
        }
        ulong_t total_followers = 0;
        for ( ulong_t r=0; r<NODES; r++ )
            total_followers += nodes[r].followers;
        return total_followers;
    }

#endif
//##############################################################################
//##############################################################################
#ifdef sgraph_page_rank

    void reset_graph_stats()
    {
        for ( ulong_t i=0; i< NODES; i++ )
        {
            nodes[i].page_rank = 0;
            nodes[i].next_rank = 0;
        }
    }

    void print_graph()
    {
        for ( ulong_t i=0; i<NODES; i++ )
            cout << "Node: " << i << " OutDegree:" << nodes[i].out_degree << " PageRank:" << nodes[i].page_rank << " NextRank: " << nodes[i].next_rank << endl;
    }

    ulong_t run_golden()
    {
        for ( ulong_t i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / NODES;
            nodes[i].next_rank = 0.15 / NODES;
        }
        ulong_t count = 0;
        float diff = 0.0;
        do {
            for ( ulong_t i=0; i<NODES; i++ )
            {
                float delta = 0.85 * nodes[i].page_rank / nodes[i].out_degree;
                for ( ulong_t j=0; j<nodes[i].out_degree; j++ ) // for node.successors
                        nodes[i].successors[j]->next_rank += delta;
            }
            diff = 0.0;
            for ( ulong_t i=0; i<NODES; i++ )
            {
                diff += fabsf(nodes[i].next_rank - nodes[i].page_rank);
                nodes[i].page_rank = nodes[i].next_rank;
                nodes[i].next_rank = 0.15 / NODES;
            }
            cout << "ITERATION: " << count << " ERROR: " << diff << endl;
            //print_graph();
        } while (++count < PAGERANK_MAX_ITERATIONS && diff > PAGERANK_MAX_ERROR);

        return (ulong_t)(diff * 1000000.0); // Convert error to fixed point integer
    }

#endif
//##############################################################################
//##############################################################################
#ifdef sgraph_bfs

    ulong_t* queue;    // queue for BFS search
    ulong_t  head, tail, elements;

    void reset_graph_stats()
    {
        for ( ulong_t i=1; i< NODES; i++ )
        {
            nodes[i].distance = NC;
        }
        nodes[0].distance = 0;       // node[0] is the starting node
        queue_init;                  // Initialize queue
    }

    void print_graph()
    {
        for ( ulong_t i=0; i<NODES; i++ )
            cout << "Node: " << i << " Distance:" << nodes[i].distance << endl;
    }

    ulong_t run_golden()
    {
        // cout << "Visited 0 with distance 0" << endl;
        ulong_t total_distance = 0;

        for (int x=0; x<BFS_MAX_ITERATIONS; x++)
        {
            nodes[x].distance = 0;       // node[0] is the starting node
            queue_init;                  // Initialize queue
            queue_push(&nodes[x]);               // Push the first element
            while ( !(queue_empty) )
            {
                node* v = (node*)queue_top;
                queue_pop;
                for ( ulong_t c=0; c<v->out_degree ; c++ )
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

#endif
//##############################################################################
//##############################################################################
#ifdef sgraph_bellman_ford

    ulong_t  weights_size;     // Size of the weights list
    ulong_t* weights_list;     // List of the weights (not used directly)

    void reset_graph_stats()
    {
        for ( ulong_t i=1; i< NODES; i++ )
        {
            nodes[i].distance = NC; // Infinity
        }
        nodes[0].distance = 0;       // node[0] is the starting node
    }

    void print_graph()
    {
        for ( ulong_t i=0; i<NODES; i++ )
            cout << "Node: " << i << " Distance:" << nodes[i].distance << endl;
        for ( ulong_t r=0; r<NODES; r++ )
        {
            cout << "Node " << r << ".weights: [ ";
            for ( ulong_t c=0; c< nodes[r].out_degree; c++ )
            {
                cout << nodes[r].weights[c] << " ";
            }
            cout << "]" << endl;
        }        
    }

    ulong_t run_golden()
    {
        ulong_t checksum = 0;
        for ( unsigned r=0; r<NODES; r++ )
            for ( ulong_t c=0; c<nodes[r].out_degree; c++ ) // for node.successors
            {
                node*u = &nodes[r];
                node*v = nodes[r].successors[c];
                ulong_t w = nodes[r].weights[c];
                if ( u->distance != NC && v->distance > u->distance + w )
                    v->distance = u->distance + w;
                checksum += w;
            }
        return checksum;
    }

#endif
//##############################################################################
//##############################################################################
//##############################################################################
//##############################################################################

void create_graph()
{
    // Initialize the list of lists
    // Later, we should read the graph from data sets
    nodes = (node*)allocate_region(NODES*sizeof(node));
    #ifdef sgraph_bfs
    queue = (ulong_t*)allocate_region(NODES*sizeof(ulong));
    #endif
    successors_size = 0;
    successors_list = NULL;

    ASSERT_DBG(MAX_OUTDEGREE < NODES);

    for ( ulong_t i=0; i<NODES; i++ )
    {
        ulong_t num_succ = (i%step==0)?(rand()%MAX_OUTDEGREE):(0);
        nodes[i].out_degree = num_succ;
        nodes[i].ID = i;
        if ( num_succ == 0 )
            nodes[i].successors = NULL;
        else
        {
            nodes[i].successors = (node**)allocate_region(num_succ*sizeof(node*)); 
            successors_size += num_succ*sizeof(node*);
        }

        /* fill the successor list of node[i] */
        ulong_t succ = rand()%NODES; // Random;
        ASSERT_DBG(MAX_OUTDEGREE < 500);
        for ( ulong_t j=0; j<num_succ; j++ )
        {
            while (succ==i)
            {
                succ += (rand()%(NODES/500) + 1);
                if ( succ >= NODES )
                    succ -= NODES;
            }
            /* we have found a new successor which we are not already connected to */
            nodes[i].successors[j] = &nodes[succ];
        }

        // Kernel specific initializations
        #ifdef sgraph_teenage_follower
        nodes[i].teenager = (rand()%2)?true:false;
        #endif
    }

    #ifdef sgraph_bellman_ford
    cout << " Initializing weights_list for the weighted graph ..." << endl;
    weights_size = 0;
    weights_list = NULL;

    for ( ulong_t i=0; i<NODES; i++ )
    {
        ulong_t d = nodes[i].out_degree;
        if ( d == 0 )
            nodes[i].weights = NULL;
        else
        {
            nodes[i].weights = (ulong_t*)allocate_region(d*sizeof(ulong_t));
            weights_size += d*sizeof(ulong_t);
        }
        for ( ulong_t j=0; j<d; j++)
            nodes[i].weights[j] = rand()%MAX_WEIGHT + 1;
    }
    
    weights_list = nodes[0].weights;
    #endif

    successors_list = nodes[0].successors;
    reset_graph_stats();
}

void print_list_of_lists()
{
    for ( ulong_t r=0; r<NODES; r++ )
    {
        cout << "Node " << r << " [ ";
        for ( ulong_t c=0; c< nodes[r].out_degree; c++ )
        {
            cout << nodes[r].successors[c]->ID << " ";
        }
        cout << "]" << endl;
    }
}

#endif