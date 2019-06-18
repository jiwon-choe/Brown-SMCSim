
#include "defs.hh"
#include "kernel_params.h"
#define ELEMENT(M, S, r, c) M[r*S+c]

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        unsigned r, c;
        ulong_t NODES;
        ulong_t total_distance;
        node* nodes;
        ulong_t* adj;
        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: adj matrix
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        adj = (ulong_t*)PIM_SREG[1];
        NODES = PIM_SREG[2];
        pim_print_hex("NODES", NODES);

        total_distance = 0;
        for ( r=0; r<NODES; r++ )
            for ( c=0; c< NODES; c++ )
                if ( ELEMENT(adj, NODES, r, c) != NC ) // We found an edge
                {
                    node*u = &nodes[r];
                    node*v = &nodes[c];
                    if ( u->distance != NC && v->distance > u->distance + ELEMENT(adj, NODES, r, c) )
                        v->distance = u->distance + ELEMENT(adj, NODES, r, c);
                }

        // Now find the total distance and return it (as a checksum to compare with PIM)
        for ( r=0; r<NODES; r++ )
            total_distance += nodes[r].distance;

        pim_print_hex("total_distance", total_distance);
        PIM_SREG[3] = total_distance;
    }

#endif