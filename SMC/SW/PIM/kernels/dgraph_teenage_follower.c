
#include "defs.hh"
#include "kernel_params.h"
#define ELEMENT(M, S, r, c) M[r*S+c]

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        unsigned r, c;
        ulong_t NODES;
        ulong_t total_followers;
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
        total_followers = 0;
        for ( r=0; r<NODES; r++ )
        {
            if ( nodes[r].teenager)
                for ( c=0; c< NODES; c++ )
                {
                    if ( ELEMENT(adj, NODES, r, c) != NC )
                    {
                        #ifdef USE_HMC_ATOMIC_CMD
                        HMC_ATOMIC___INCR(nodes[c].followers);
                        #else
                        nodes[c].followers++;
                        #endif
                    }
                }
        }
        for ( r=0; r<NODES; r++ )
        {
            total_followers += nodes[r].followers;
        }

        // pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif

/******************************************************************************/
#ifdef kernel_dma1

// This code uses only one DMA resource in Ping-pong mode
    #define nodes_ping    ((node*)(ping+XFER_SIZE*0))
    #define nodes_pong    ((node*)(pong+XFER_SIZE*0))
    #define nodes_count   (XFER_SIZE/sizeof(node))
    #define nodes_chunk   (((XFER_SIZE)/sizeof(node))*sizeof(node))

    void execute_kernel()
    {
        unsigned r, c, rr;
        ulong_t NODES;
        ulong_t total_followers;
        node* nodes;
        ulong_t* adj;
        volatile uint8_t* ping;
        volatile uint8_t* pong;
        volatile uint8_t* swap;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: adj matrix
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        adj = (ulong_t*)PIM_SREG[1];
        NODES = PIM_SREG[2];

        ping = &PIM_VREG[0];
        pong = &PIM_VREG[0] + MAX_XFER_SIZE*2;
        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);

        // At first, fill the ping buffers to work with them
        DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

        total_followers = 0;
        rr = nodes_count;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count ) // TODO: I am not checking for the boundary condition and I'm hoping for the best
            {
                rr = 0;
                DMA_WAIT( DMA_RES0);
                swap = ping; ping = pong; pong = swap; // Swap Ping and Pong
                DMA_REQUEST(&nodes[r+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            if ( nodes_ping[rr].teenager)
                for ( c=0; c< NODES; c++ )
                {
                    if ( ELEMENT(adj, NODES, r, c) != NC )
                    {
                        #ifdef USE_HMC_ATOMIC_CMD
                        HMC_ATOMIC___INCR(nodes[c].followers);
                        #else
                        nodes[c].followers++;
                        #endif
                    }
                }
            rr++;
        }
        for ( r=0; r<NODES; r++ )
        {
            total_followers += nodes[r].followers;
        }

        // pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif

/******************************************************************************/
#ifdef kernel_dma2
/*  This code uses two DMA resources:
        pi0, po0: ping pong buffers related to the nodes structure
        pi1, po1: ping pong buffers related to the adj matrix
    */
    #define nodes_pi0    ((node*)(pi0))
    #define nodes_po0    ((node*)(po0))
    #define adj_pi1      ((ulong_t*)(pi1))
    #define adj_po1      ((ulong_t*)(po1))
    #define nodes_count  (XFER_SIZE/sizeof(node))
    #define nodes_chunk  (((XFER_SIZE)/sizeof(node))*sizeof(node))
    #define adj_count    (XFER_SIZE/sizeof(ulong_t))
    #define adj_chunk    (((XFER_SIZE)/sizeof(ulong_t))*sizeof(ulong_t))

    void execute_kernel()
    {
        unsigned r, c, rr, cc;
        ulong_t NODES;
        ulong_t total_followers;
        node* nodes;
        ulong_t* adj;
        volatile uint8_t* pi0;
        volatile uint8_t* po0;
        volatile uint8_t* pi1;
        volatile uint8_t* po1;
        volatile uint8_t* swap;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: adj matrix
        SREG[2]: number of nodes
        SREG[3]: retval
        */
        nodes = (node*)PIM_SREG[0];
        adj = (ulong_t*)PIM_SREG[1];
        NODES = PIM_SREG[2];

        pi0 = &PIM_VREG[0] + MAX_XFER_SIZE*0;
        po0 = &PIM_VREG[0] + MAX_XFER_SIZE*1;
        pi1 = &PIM_VREG[0] + MAX_XFER_SIZE*2;
        po1 = &PIM_VREG[0] + MAX_XFER_SIZE*3;

        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        pim_print_hex("nodes_count", nodes_count);
        pim_print_hex("adj_chunk", adj_chunk);
        pim_print_hex("adj_count", adj_count);

        total_followers = 0;
        /*******DMA TRANSFER*******/
        // At first, fill the pi0 buffers to work with them
        DMA_REQUEST(nodes, nodes_po0, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
        
        rr = nodes_count;
        for ( r=0; r<NODES; r++ )
        {
            if ( rr == nodes_count )
            {
                rr = 0;
                DMA_WAIT( DMA_RES0);
                swap = pi0; pi0 = po0; po0 = swap; // Swap pi0 and po0
                if ( r+nodes_count<NODES ) // Boundary Condition
                    DMA_REQUEST(&nodes[r+nodes_count], nodes_po0, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            }

            if ( nodes_pi0[rr].teenager)
            {
                /*******DMA TRANSFER*******/
                DMA_REQUEST(&adj[r*NODES], adj_po1, adj_chunk, PIM_DMA_READ, DMA_RES1 );
                //pim_print_hex("***r***", r);
                cc = adj_count;
                for ( c=0; c< NODES; c++ )
                {
                    if ( cc == adj_count )
                    {
                        cc = 0;
                        DMA_WAIT( DMA_RES1);
                        swap = pi1; pi1 = po1; po1 = swap; // Swap pi1 and po1
                        if ( c+adj_count<NODES ) // Boundary Condition
                            DMA_REQUEST((&adj[r*NODES+c+adj_count]), adj_po1, adj_chunk, PIM_DMA_READ, DMA_RES1 );
                    }
                    //pim_print_hex("ADJ[c]", adj_pi1[cc]);
                    if ( adj_pi1[cc] != NC )
                    {
                        #ifdef USE_HMC_ATOMIC_CMD
                        HMC_ATOMIC___INCR(nodes[c].followers);
                        #else
                        nodes[c].followers++;
                        #endif
                    }
                    cc++;
                }
            }
            rr++;
        }
        for ( r=0; r<NODES; r++ )
        {
            total_followers += nodes[r].followers;
        }
        // pim_print_hex("total_followers", total_followers);
        PIM_SREG[3] = total_followers;
    }

#endif
