#ifndef UTILS_H_DGRAPH
#define UTILS_H_DGRAPH

#include "_app_params.h"
#include "defs.hh"
#include <math.h>

#define MATRIX_TYPE  ulong_t(*)[NODES]
node* nodes;          // Graph nodes
void* adj;

//##############################################################################
//##############################################################################
#ifdef dgraph_teenage_follower

    void reset_graph_stats()
    {
        for ( unsigned i=0; i< NODES; i++ )
        {
            nodes[i].followers = 0;
        }
    }

    ulong_t run_golden()
    {
        for ( unsigned r=0; r<NODES; r++ )
        {
            if ( nodes[r].teenager )
                for ( unsigned c=0; c< NODES; c++ )
                {
                    if ( ((MATRIX_TYPE)adj)[r][c] != NC )
                        nodes[c].followers++;
                }
        }
        ulong_t total_followers = 0;
        for ( unsigned r=0; r<NODES; r++ )
            total_followers += nodes[r].followers;
        return total_followers;
    }

    void print_graph()
    {
        for ( unsigned i=0; i<NODES; i++ )
            cout << "Node: " << i << " Teenager:" << nodes[i].teenager << " Followers: " << nodes[i].followers << endl;
    }

#endif
//##############################################################################
//##############################################################################
#ifdef dgraph_page_rank

    void reset_graph_stats()
    {
        for ( unsigned i=0; i< NODES; i++ )
        {
            nodes[i].page_rank = 0;
            nodes[i].next_rank = 0;
        }
    }

    void print_graph()
    {
        for ( unsigned i=0; i<NODES; i++ )
            cout << "Node: " << i << " OutDegree:" << nodes[i].out_degree << " PageRank:" << nodes[i].page_rank << " NextRank: " << nodes[i].next_rank << endl;
    }

    ulong_t run_golden()
    {
        for ( unsigned i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / NODES;
            nodes[i].next_rank = 0.15 / NODES;
        }
        ulong_t count = 0;
        float diff = 0.0;
        do {
            for ( unsigned i=0; i<NODES; i++ )
            {
                float delta = 0.85 * nodes[i].page_rank / nodes[i].out_degree;
                for ( unsigned j=0; j<NODES; j++ ) // for node.successors
                    if ( ((MATRIX_TYPE)adj)[i][j] != NC ) // if c is a successor
                        nodes[j].next_rank += delta;
            }
            diff = 0.0;
            for ( unsigned i=0; i<NODES; i++ )
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
#ifdef dgraph_bellman_ford

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
    }

    ulong_t run_golden()
    {
        ulong_t total_distance = 0;
        for ( unsigned r=0; r<NODES; r++ )
            for ( unsigned c=0; c< NODES; c++ )
                if ( ((MATRIX_TYPE)adj)[r][c] != NC ) // We found an edge
                {
                    node*u = &nodes[r];
                    node*v = &nodes[c];
                    if ( u->distance != NC && v->distance > u->distance + ((MATRIX_TYPE)adj)[r][c] )
                        v->distance = u->distance + ((MATRIX_TYPE)adj)[r][c];
                }
        // Now find the total distance and return it (as a checksum to compare with PIM)
        for ( unsigned r=0; r<NODES; r++ )
            total_distance += nodes[r].distance;
        return total_distance;
    }

#endif
//##############################################################################
//##############################################################################
//##############################################################################
//##############################################################################
void create_graph()
{
    nodes = (node*) allocate_region(NODES*sizeof(node));
    adj = allocate_region(NODES*NODES*sizeof(ulong_t));

    // Initialize adjacency matrix randomly
    for ( unsigned r=0; r<NODES; r++ )
    {
        for ( unsigned c=0; c< NODES; c++ )
        {
            unsigned d = rand()%100 + 1;
            if ( (d <= DENSITY) && (r != c) )
                ((MATRIX_TYPE)adj)[r][c] = rand()%MAX_WEIGHT + 1;
            else
                ((MATRIX_TYPE)adj)[r][c] = NC; // Not connected
        }
    }

    for ( unsigned i=0; i<NODES; i++ )
    {
        nodes[i].teenager = (rand()%2)?true:false;
    }

    cout << "Calculating out_degree (once for all nodes) ..." << endl;
    for ( unsigned r=0; r< NODES; r++ )
    {
        nodes[r].out_degree = 0;
        for ( unsigned c=0; c< NODES; c++ )
            if (((MATRIX_TYPE)adj)[r][c] != NC)
                nodes[r].out_degree +=1;
    }

    reset_graph_stats();
}

void print_adj_matrix()
{
    ios::fmtflags f( cout.flags() );
    // Initialize adjacency matrix randomly
    cout << setw(6) << "*";
    for ( unsigned c=0; c< NODES; c++ )
    {
        cout << setw(6) << c;
    }
    cout << endl;

    for ( unsigned r=0; r<NODES; r++ )
    {
        cout << setw(6) << r;
        for ( unsigned c=0; c< NODES; c++ )
        {
            if ( ((MATRIX_TYPE)adj)[r][c] == NC )
                cout << setw(6) << "-";
            else
                cout << setw(6) << ((MATRIX_TYPE)adj)[r][c];
        }
        cout << endl;
    }
  cout.flags( f );    
}

#endif