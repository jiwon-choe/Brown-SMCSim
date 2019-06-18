#include "defs.hh"
#include "kernel_params.h"

// Number of nodes fitting in one DMA transfer, and the chunk size in bytes
#define nodes_count   (XFER_SIZE/sizeof(node))
#define nodes_chunk   (((XFER_SIZE)/sizeof(node))*sizeof(node))
const uint8_t DMA_RES[] = {DMA_RES0, DMA_RES1, DMA_RES2, DMA_RES3, DMA_RES4, DMA_RES5, DMA_RES6};

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        ulong_t r, c;
        ulong_t NODES;
        node* nodes;
        ulong_t checksum;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pim_print_hex("NODES", NODES);
        checksum = 0;

        for ( r=0; r<NODES; r++ )
            for ( c=0; c<nodes[r].out_degree; c++ ) // for node.successors
            {
                node*u = &nodes[r];
                node*v = nodes[r].successors[c];
                ulong_t w = nodes[r].weights[c];
                if ( u->distance != NC && v->distance > u->distance + w )
                {
                    v->distance = u->distance + w;
                    checksum += w;
                }
            }

        pim_print_hex("checksum", checksum);
        PIM_SREG[3] = checksum;
    }

#endif


/******************************************************************************/
#ifdef kernel_atomicmin

    void execute_kernel()
    {
        ulong_t r, c, rr;
        ulong_t NODES;
        node* nodes;
        ulong_t checksum;
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;

        // volatile node* u;
        // volatile node* v;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        checksum = 0;
        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);

        // Initialization phase
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        rr = nodes_count;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition
            {
                rr = 0;
                DMA_WAIT(DMA_RES0);
                nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            for ( c=0; c<nodes_ping[rr].out_degree; c++ ) // for node.successors
            {
                HMC_OPERAND = nodes_ping[rr].distance + nodes_ping[rr].weights[c];
                HMC_ATOMIC___IMIN(nodes_ping[rr].successors[c]->distance);
                checksum += nodes_ping[rr].weights[c];
            }
            rr++;
        }
        pim_print_hex("checksum", checksum);
        PIM_SREG[3] = checksum;
    }

#endif

/******************************************************************************/
#ifdef kernel_simpledma

    void execute_kernel()
    {
        ulong_t r, c, rr;
        ulong_t NODES;
        node* nodes;
        ulong_t checksum;
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;
        volatile node**   succ;
        volatile ulong_t* weights;

        // volatile node* u;
        // volatile node* v;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        checksum = 0;
        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);
        succ       = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        weights    = (volatile ulong_t*)(&PIM_VREG[0] + MAX_XFER_SIZE*3);

        // Initialization phase
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        rr = nodes_count;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition
            {
                rr = 0;
                DMA_WAIT(DMA_RES0);
                nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            /* Fetch successors list */
            if ( nodes_ping[rr].out_degree )
            {
                DMA_REQUEST(nodes_ping[rr].successors, succ, nodes_ping[rr].out_degree * sizeof(node*),   PIM_DMA_READ, DMA_RES1 );
                DMA_REQUEST(nodes_ping[rr].weights, weights, nodes_ping[rr].out_degree * sizeof(ulong_t), PIM_DMA_READ, DMA_RES2 );
            }

            DMA_WAIT((DMA_RES1 | DMA_RES2));

            for ( c=0; c<nodes_ping[rr].out_degree; c++ ) // for node.successors
            {
                HMC_OPERAND = nodes_ping[rr].distance + weights[c];
                HMC_ATOMIC___IMIN(succ[c]->distance);
                checksum += weights[c];
            }
            rr++;
        }
        pim_print_hex("checksum", checksum);
        PIM_SREG[3] = checksum;
    }

#endif

/******************************************************************************/
#ifdef kernel_best

    void execute_kernel()
    {
        ulong_t r, c, rr;
        ulong_t NODES;
        node* nodes;
        ulong_t checksum;
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;
        volatile node**   succ_ping;
        volatile node**   succ_pong;
        volatile node**   succ_swap;
        volatile ulong_t* weights_ping;
        volatile ulong_t* weights_pong;
        volatile ulong_t* weights_swap;
        int num_pongs;
        unsigned curr_distance;

        // volatile node* u;
        // volatile node* v;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        checksum = 0;
        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);
        succ_ping       = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        weights_ping    = (volatile ulong_t*)(&PIM_VREG[0] + MAX_XFER_SIZE*3);
        succ_pong       = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*4);
        weights_pong    = (volatile ulong_t*)(&PIM_VREG[0] + MAX_XFER_SIZE*5);

        // Initialization phase
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        rr = nodes_count;
        num_pongs = -1;
        curr_distance = 0;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition
            {
                rr = 0;
                DMA_WAIT(DMA_RES0);
                nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }


            /* Fetch successors list */
            if ( nodes_ping[rr].out_degree )
            {
                DMA_REQUEST(nodes_ping[rr].successors, succ_pong, nodes_ping[rr].out_degree * sizeof(node*),   PIM_DMA_READ, DMA_RES1 );
                DMA_REQUEST(nodes_ping[rr].weights, weights_pong, nodes_ping[rr].out_degree * sizeof(ulong_t), PIM_DMA_READ, DMA_RES2 );
            }

            if ( num_pongs != -1 )
            {
                for ( c=0; c<num_pongs; c++ ) // for node.successors
                {
                    #ifdef USE_HMC_ATOMIC_CMD
                    HMC_OPERAND = curr_distance + weights_ping[c];
                    HMC_ATOMIC___IMIN(succ_ping[c]->distance);
                    #else
                    if (curr_distance + weights_ping[c] < succ_ping[c]->distance)
                        succ_ping[c]->distance = curr_distance + weights_ping[c];
                    #endif
                    checksum += weights_ping[c];
                }
                num_pongs = -1;
            }

            if ( nodes_ping[rr].out_degree )
            {
                num_pongs = nodes_ping[rr].out_degree;
                curr_distance = nodes_ping[rr].distance;
                DMA_WAIT((DMA_RES1 | DMA_RES2));
                succ_swap = succ_ping; succ_ping = succ_pong; succ_pong = succ_swap;
                weights_swap = weights_ping; weights_ping = weights_pong; weights_pong = weights_swap;
            }

            rr++;
        }

        if ( num_pongs != -1 )
        {
            for ( c=0; c<num_pongs; c++ ) // for node.successors
            {
                HMC_OPERAND = curr_distance + weights_ping[c];
                #ifdef USE_HMC_ATOMIC_CMD
                HMC_ATOMIC___IMIN(succ_ping[c]->distance);
                #else
                if (curr_distance + weights_ping[c] < succ_ping[c]->distance)
                    succ_ping[c]->distance = curr_distance + weights_ping[c];
                #endif
                checksum += weights_ping[c];
            }
            num_pongs = -1;
        }
        
        pim_print_hex("checksum", checksum);
        PIM_SREG[3] = checksum;
    }

#endif