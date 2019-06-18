
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
        unsigned r, c;
        ulong_t NODES;
        ulong_t total_followers;
        node* nodes;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];

        pim_print_hex("NODES", NODES);
        total_followers = 0;
        for ( r=0; r<NODES; r++ )
        {
            if ( nodes[r].teenager)
            {
                for ( c=0; c< nodes[r].out_degree; c++ )
                {
                    node*succ = nodes[r].successors[c];
                    #ifdef USE_HMC_ATOMIC_CMD
                    HMC_ATOMIC___INCR(succ->followers);
                    #else
                    succ->followers++;
                    #endif
                }
            }
        }
        for ( r=0; r<NODES; r++ )
            total_followers += nodes[r].followers;

        pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif

/******************************************************************************/
#ifdef kernel_dma1

    void execute_kernel()
    {
        unsigned r, c, rr;
        ulong_t NODES;
        ulong_t total_followers;
        node* nodes;

        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);

        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);

        // Initialization phase
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        total_followers = 0;
        rr = nodes_count;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition
            {
                rr = 0;
                DMA_WAIT( DMA_RES0);
                nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            if ( nodes_ping[rr].teenager)
            {
                for ( c=0; c< nodes_ping[rr].out_degree; c++ )
                {
                    node*succ = nodes_ping[rr].successors[c];       // TODO: FETCH THIS WITH DMA TOO
                    #ifdef USE_HMC_ATOMIC_CMD
                    HMC_ATOMIC___INCR(succ->followers);
                    #else
                    succ->followers++;
                    #endif
                }
            }
            rr++;
        }
        for ( r=0; r<NODES; r++ )
            total_followers += nodes[r].followers;

        pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif

/******************************************************************************/
#ifdef kernel_best

    void execute_kernel()
    {
        unsigned r, c, rr;
        ulong_t NODES;
        ulong_t total_followers;
        node* nodes;

        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;
        volatile node** succ_ping;
        volatile node** succ_pong;
        volatile node** succ_swap;
        int num_pongs;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        nodes_ping = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*0);
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*1);
        succ_ping = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        succ_pong = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*3);

        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);

        // Initialization phase
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        total_followers = 0;
        rr = nodes_count;
        num_pongs = -1;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition
            {
                rr = 0;
                DMA_WAIT( DMA_RES0);
                nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            if ( nodes_ping[rr].teenager && nodes_ping[rr].out_degree )
            {
                DMA_REQUEST(nodes_ping[rr].successors, succ_pong, nodes_ping[rr].out_degree * sizeof(node*), PIM_DMA_READ, DMA_RES1 );
            }

            if ( num_pongs != -1 )
            {
                for ( c=0; c< num_pongs; c++ )
                {
                    #ifdef USE_HMC_ATOMIC_CMD
                    HMC_ATOMIC___INCR(succ_ping[c]->followers);
                    #else
                    succ_ping[c]->followers++;
                    #endif
                }
                num_pongs = -1;
            }

            if ( nodes_ping[rr].teenager && nodes_ping[rr].out_degree )
            {
                num_pongs = nodes_ping[rr].out_degree;
                DMA_WAIT( DMA_RES1);
                succ_swap = succ_ping; succ_ping = succ_pong; succ_pong = succ_swap;
            }
            rr++;
        }

        // Termination
        if ( num_pongs != -1 )
        {
            for ( c=0; c< num_pongs; c++ )
            {
                #ifdef USE_HMC_ATOMIC_CMD
                HMC_ATOMIC___INCR(succ_ping[c]->followers);
                #else
                succ_ping[c]->followers++;
                #endif
            }
            num_pongs = -1;
        }

        for ( r=0; r<NODES; r++ )
            total_followers += nodes[r].followers;

        pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif

