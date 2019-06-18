
#include "defs.hh"
#include "kernel_params.h"

#define nodes_count   (XFER_SIZE/sizeof(node))
#define nodes_chunk   (((XFER_SIZE)/sizeof(node))*sizeof(node))

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        unsigned i, j;
        ulong_t NODES, MAX_ITERATIONS, count;
        node* nodes;
        float diff, delta, f, MAX_ERROR;
        float* pf;
        #ifdef USE_HMC_ATOMIC_CMD
        uint32_t* t;
        #endif
        
        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pf = (float*)(&(PIM_SREG[4]));
        MAX_ERROR = *(pf);
        MAX_ITERATIONS = PIM_SREG[5];

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_msg("MAX_ERROR:");
        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = PIM_SREG[4];
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        for ( i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / (float)NODES;
            nodes[i].next_rank = 0.15 / (float)NODES;
        }
        count = 0;
        diff = 0.0;
        do {
            for ( i=0; i<NODES; i++ )
            {
                delta = 0.85 * nodes[i].page_rank / (float)nodes[i].out_degree;
                #ifdef USE_HMC_ATOMIC_CMD
                t = (uint32_t*)&delta;
                HMC_OPERAND = *t;
                for ( j=0; j<nodes[i].out_degree; j++ ) // for node.successors
                    HMC_ATOMIC___FADD(nodes[i].successors[j]->next_rank);
                #else
                for ( j=0; j<nodes[i].out_degree; j++ ) // for node.successors
                    nodes[i].successors[j]->next_rank += delta;
                #endif
            }
            diff = 0.0;
            for ( i=0; i<NODES; i++ )
            {
                /* absolute value */
                f = nodes[i].next_rank - nodes[i].page_rank;
                if ( f < 0 )
                    f = -f;
                diff += f;
                nodes[i].page_rank = nodes[i].next_rank;
                nodes[i].next_rank = 0.15 / (float)NODES;
            }
            #ifdef DEBUG_RESIDENT
            pim_print_hex("iteration", count);
            pim_print_msg("error:");
            PIM_SREG[0] = *((ulong_t*)&diff);
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif
        } while (++count < MAX_ITERATIONS && diff > MAX_ERROR);
        PIM_SREG[3] = (ulong_t)(diff * 1000000.0);
    }

#endif

/******************************************************************************/
/* DMA is used only for the first loop, so only DMA READ is required */
#ifdef kernel_dma1

    void execute_kernel()
    {
        unsigned i, j, rr;
        ulong_t NODES, MAX_ITERATIONS, count;
        node* nodes;
        float diff, delta, f, MAX_ERROR;
        float* pf;
        uint32_t* t;

        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pf = (float*)(&(PIM_SREG[4]));
        MAX_ERROR = *(pf);
        MAX_ITERATIONS = PIM_SREG[5];
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_msg("MAX_ERROR:");
        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = PIM_SREG[4];
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        for ( i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / (float)NODES;
            nodes[i].next_rank = 0.15 / (float)NODES;
        }

        count = 0;
        diff = 0.0;
        do {

            // Initialization phase
            DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            for ( i=0; i<NODES; i++ )
            {

                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( DMA_RES0);
                    nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                }

                delta = 0.85 * nodes_ping[rr].page_rank / (float)nodes_ping[rr].out_degree;
                #ifdef USE_HMC_ATOMIC_CMD
                t = (uint32_t*)&delta;
                HMC_OPERAND = *t;
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    HMC_ATOMIC___FADD(nodes_ping[rr].successors[j]->next_rank);
                #else
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    nodes_ping[rr].successors[j]->next_rank += delta;
                #endif
                rr++;
            }
            diff = 0.0;
            for ( i=0; i<NODES; i++ )
            {
                /* absolute value */
                f = nodes[i].next_rank - nodes[i].page_rank;
                if ( f < 0 )
                    f = -f;
                diff += f;
                nodes[i].page_rank = nodes[i].next_rank;
                nodes[i].next_rank = 0.15 / (float)NODES;
            }
            #ifdef DEBUG_RESIDENT
            pim_print_hex("iteration", count);
            pim_print_msg("error:");
            PIM_SREG[0] = *((ulong_t*)&diff);
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif
        } while (++count < MAX_ITERATIONS && diff > MAX_ERROR);
        PIM_SREG[3] = (ulong_t)(diff * 1000000.0);
    }

#endif

/******************************************************************************/
/* DMA is used in both first and second loops. In the second loop, we need to 
   READ using DMA and then WRITE back the dirty block using DMA */
#ifdef kernel_dma2

    void execute_kernel()
    {
        unsigned i, j, rr;
        ulong_t NODES, MAX_ITERATIONS, count, last_chunk;
        node* nodes;
        float diff, delta, f, MAX_ERROR;
        float* pf;
        uint32_t* t;
        bool first;

        /* First loop*/
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;

        /* Second loop*/
        volatile node* nodes_R; // Nodes to READ
        volatile node* nodes_P; // Nodes to PROCESS
        volatile node* nodes_W; // Nodes to WRITE

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pf = (float*)(&(PIM_SREG[4]));
        MAX_ERROR = *(pf);
        MAX_ITERATIONS = PIM_SREG[5];
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);

        nodes_R = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        nodes_P = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*3);
        nodes_W = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*4);

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_msg("MAX_ERROR:");
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        pim_print_hex("nodes_count", nodes_count);

        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = PIM_SREG[4];
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        for ( i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / (float)NODES;
            nodes[i].next_rank = 0.15 / (float)NODES;
        }

        count = 0;
        diff = 0.0;
        do {

            // Initialization phase
            DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            for ( i=0; i<NODES; i++ )
            {
                /* Swap ping and pong */
                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( DMA_RES0);
                    nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                }

                /* Process the ping buffer */
                delta = 0.85 * nodes_ping[rr].page_rank / (float)nodes_ping[rr].out_degree;
                #ifdef USE_HMC_ATOMIC_CMD
                t = (uint32_t*)&delta;
                HMC_OPERAND = *t;
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    HMC_ATOMIC___FADD(nodes_ping[rr].successors[j]->next_rank);
                #else
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    nodes_ping[rr].successors[j]->next_rank += delta;
                #endif
                rr++;
            }

            // R = READ, P = PROCESS, W = WRITE
            // R --> P --> W --> ...
            // P --> W --> R --> ...
            // W --> R --> P --> ...

            DMA_WAIT( DMA_RES0); // Wait in case previous loop is still using the DMA
            DMA_REQUEST(nodes, nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            diff = 0.0;
            first = true;
            for ( i=0; i<NODES; i++ )
            {
                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( (DMA_RES0 | DMA_RES1) ); // Wait for READ and WRITE to complete
                    nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                    if ( ! first )
                    {
                        DMA_REQUEST(&nodes[i-nodes_count], nodes_W, nodes_chunk, PIM_DMA_WRITE, DMA_RES1 );
                    }
                    else
                        first = false;
                }

                /* absolute value */
                f = nodes_P[rr].next_rank - nodes_P[rr].page_rank;
                if ( f < 0 )
                    f = -f;
                diff += f;
                nodes_P[rr].page_rank = nodes_P[rr].next_rank;
                nodes_P[rr].next_rank = 0.15 / (float)NODES;

                rr++;
            }
            pim_print_hex("rr", rr);
            pim_print_hex("NODES-rr", NODES-rr);
            last_chunk = (rr)*sizeof(node);
            pim_print_hex("last_chunk", last_chunk);

            // Boundary condition: we have to write back the data
            nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
            DMA_REQUEST(&nodes[NODES-rr], nodes_W, last_chunk, PIM_DMA_WRITE, DMA_RES1 );
            DMA_WAIT( (DMA_RES0 | DMA_RES1) );

            #ifdef DEBUG_RESIDENT
            pim_print_hex("iteration", count);
            pim_print_msg("error:");
            PIM_SREG[0] = *((ulong_t*)&diff);
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif
        } while (++count < MAX_ITERATIONS && diff > MAX_ERROR);
        PIM_SREG[3] = (ulong_t)(diff * 1000000.0);
    }

#endif

/******************************************************************************/
/* DMA is used in both first and second loops. In the second loop, we need to 
   READ using DMA and then WRITE back the dirty block using DMA
   The parallel coprocessor is used for computations */
#ifdef kernel_coprocessor

    void execute_kernel()
    {
        unsigned i, j, rr;
        ulong_t NODES, MAX_ITERATIONS, count, last_chunk;
        node* nodes;
        volatile float diff;
        float delta, MAX_ERROR;
        volatile float* pf;
        volatile float* pdiff;
        #ifdef DEBUG_RESIDENT
        ulong_t* pint;
        #endif
        uint32_t* t;
        bool first;

        /* First loop*/
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;

        /* Second loop*/
        volatile node* nodes_R; // Nodes to READ
        volatile node* nodes_P; // Nodes to PROCESS
        volatile node* nodes_W; // Nodes to WRITE

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pf = (float*)(&(PIM_SREG[4]));
        MAX_ERROR = *(pf);
        MAX_ITERATIONS = PIM_SREG[5];
        nodes_ping = (node*)&PIM_VREG[0];
        nodes_pong = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE);

        nodes_R = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        nodes_P = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*3);
        nodes_W = (node*)(&PIM_VREG[0] + MAX_XFER_SIZE*4);

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        pim_print_hex("nodes_count", nodes_count);
        pim_print_msg("MAX_ERROR:");
        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = PIM_SREG[4];
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        for ( i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / (float)NODES;
            nodes[i].next_rank = 0.15 / (float)NODES;
        }

        count = 0;
        diff = 0.0;
        do {

            // Initialization phase
            DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            for ( i=0; i<NODES; i++ )
            {
                /* Swap ping and pong */
                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( DMA_RES0);
                    nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                }

                /* Process the ping buffer */
                delta = 0.85 * nodes_ping[rr].page_rank / (float)nodes_ping[rr].out_degree;
                #ifdef USE_HMC_ATOMIC_CMD
                t = (uint32_t*)&delta;
                HMC_OPERAND = *t;
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    HMC_ATOMIC___FADD(nodes_ping[rr].successors[j]->next_rank);
                #else
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    nodes_ping[rr].successors[j]->next_rank += delta;
                #endif
                rr++;
            }

            // R = READ, P = PROCESS, W = WRITE
            // R --> P --> W --> ...
            // P --> W --> R --> ...
            // W --> R --> P --> ...

            DMA_WAIT( DMA_RES0); // Wait in case previous loop is still using the DMA
            DMA_REQUEST(nodes, nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            diff = 0.0;
            first = true;
            for ( i=0; i<NODES; i+=nodes_count )
            {
                DMA_WAIT( (DMA_RES0 | DMA_RES1) ) ; // Wait for READ and WRITE to complete
                nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
                DMA_REQUEST(&nodes[i+nodes_count], nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                if ( ! first )
                {
                    DMA_REQUEST(&nodes[i-nodes_count], nodes_W, nodes_chunk, PIM_DMA_WRITE, DMA_RES1 );
                }
                else
                    first = false;

                /* Ask the coprocessor to do the computations*/
                PIM_SREG[0] = (ulong_t)(nodes_P);       // Pointer to the DMA buffer
                PIM_SREG[1] = nodes_count;              // Number of nodes in the DMA buffer
                PIM_SREG[2] = NODES;                    // Number of nodes in the DMA buffer
                PIM_COPROCESSOR_CMD = CMD_CUSTOM_PR1;
                /* Result is ready in PIM_SREG[3]*/
                pdiff = (float*)&(PIM_SREG[3]);
                diff += *pdiff;
            }
            pim_print_hex("rr", rr);
            pim_print_hex("NODES-rr", NODES-rr);
            last_chunk = (rr)*sizeof(node);
            pim_print_hex("last_chunk", last_chunk);
            DMA_WAIT( DMA_RES1); // Wait for WRITE to complete
            // Boundary condition: we have to write back the data
            nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
            DMA_REQUEST(&nodes[NODES-rr], nodes_W, last_chunk, PIM_DMA_WRITE, DMA_RES1 );
            DMA_WAIT( (DMA_RES0 | DMA_RES1) );

            #ifdef DEBUG_RESIDENT
            pim_print_hex("iteration", count);
            pim_print_msg("error:");
            pint = (ulong_t*)&diff;
            PIM_SREG[0] = *pint;
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif
        } while (++count < MAX_ITERATIONS && diff > MAX_ERROR);
        PIM_SREG[3] = (ulong_t)(diff * 1000000.0);
    }

#endif

/******************************************************************************/
/* This contains the most advanced use for DMA */
#ifdef kernel_best

    void execute_kernel()
    {
        unsigned i, j, rr;
        ulong_t NODES, MAX_ITERATIONS, count, last_chunk;
        node* nodes;
        float diff, delta, f, MAX_ERROR;
        float* pf;
        uint32_t* t;
        bool first;

        /* First loop*/
        volatile node* nodes_ping;
        volatile node* nodes_pong;
        volatile node* nodes_swap;
        volatile node** nodes_succ;

        /* Second loop*/
        volatile node* nodes_R; // Nodes to READ
        volatile node* nodes_P; // Nodes to PROCESS
        volatile node* nodes_W; // Nodes to WRITE

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        pf = (float*)(&(PIM_SREG[4]));
        MAX_ERROR = *(pf);
        MAX_ITERATIONS = PIM_SREG[5];
        nodes_ping = (volatile node*)(&PIM_VREG[0] + MAX_XFER_SIZE*0);
        nodes_pong = (volatile node*)(&PIM_VREG[0] + MAX_XFER_SIZE*1);
        nodes_succ = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        nodes_R    = (volatile node*)(&PIM_VREG[0] + MAX_XFER_SIZE*3);
        nodes_P    = (volatile node*)(&PIM_VREG[0] + MAX_XFER_SIZE*4);
        nodes_W    = (volatile node*)(&PIM_VREG[0] + MAX_XFER_SIZE*5);

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_msg("MAX_ERROR:");
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);
        pim_print_hex("nodes_count", nodes_count);

        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = PIM_SREG[4];
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        for ( i=0; i<NODES; i++ )
        {
            nodes[i].page_rank = 1.0 / (float)NODES;
            nodes[i].next_rank = 0.15 / (float)NODES;
        }

        count = 0;
        diff = 0.0;
        do {

            // Initialization phase
            DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            for ( i=0; i<NODES; i++ )
            {
                /* Swap ping and pong */
                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( DMA_RES0);
                    nodes_swap = nodes_ping; nodes_ping = nodes_pong; nodes_pong = nodes_swap; // Swap Ping and Pong
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                }

                /* Fetch successors list */
                if ( nodes_ping[rr].out_degree )
                    DMA_REQUEST(nodes_ping[rr].successors, nodes_succ, nodes_ping[rr].out_degree * sizeof(node*), PIM_DMA_READ, DMA_RES1 );

                /* Process the ping buffer */
                delta = 0.85 * nodes_ping[rr].page_rank / (float)nodes_ping[rr].out_degree;

                #ifdef USE_HMC_ATOMIC_CMD
                t = (uint32_t*)&delta;
                HMC_OPERAND = *t;
                DMA_WAIT( DMA_RES1);
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    HMC_ATOMIC___FADD(nodes_succ[j]->next_rank);
                #else
                DMA_WAIT( DMA_RES1);
                for ( j=0; j<nodes_ping[rr].out_degree; j++ ) // for node.successors
                    nodes_succ[j]->next_rank += delta;
                #endif
                rr++;
            }

            // R = READ, P = PROCESS, W = WRITE
            // R --> P --> W --> ...
            // P --> W --> R --> ...
            // W --> R --> P --> ...

            DMA_WAIT( DMA_RES0); // Wait in case previous loop is still using the DMA
            DMA_REQUEST(nodes, nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
            rr = nodes_count;
            diff = 0.0;
            first = true;
            for ( i=0; i<NODES; i++ )
            {
                if ( rr == nodes_count )
                {
                    rr = 0;
                    DMA_WAIT( (DMA_RES0 | DMA_RES1) ); // Wait for READ and WRITE to complete
                    nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
                    DMA_REQUEST(&nodes[i+nodes_count], nodes_R, nodes_chunk, PIM_DMA_READ, DMA_RES0 );
                    if ( ! first )
                    {
                        DMA_REQUEST(&nodes[i-nodes_count], nodes_W, nodes_chunk, PIM_DMA_WRITE, DMA_RES1 );
                    }
                    else
                        first = false;
                }

                /* absolute value */
                f = nodes_P[rr].next_rank - nodes_P[rr].page_rank;
                if ( f < 0 )
                    f = -f;
                diff += f;
                nodes_P[rr].page_rank = nodes_P[rr].next_rank;
                nodes_P[rr].next_rank = 0.15 / (float)NODES;

                rr++;
            }
            pim_print_hex("rr", rr);
            pim_print_hex("NODES-rr", NODES-rr);
            last_chunk = (rr)*sizeof(node);
            pim_print_hex("last_chunk", last_chunk);

            // Boundary condition: we have to write back the data
            nodes_swap = nodes_W; nodes_W = nodes_P; nodes_P = nodes_R; nodes_R = nodes_swap;
            DMA_REQUEST(&nodes[NODES-rr], nodes_W, last_chunk, PIM_DMA_WRITE, DMA_RES1 );
            DMA_WAIT((DMA_RES0 | DMA_RES1 ));

            #ifdef DEBUG_RESIDENT
            pim_print_hex("iteration", count);
            pim_print_msg("error:");
            PIM_SREG[0] = *((ulong_t*)&diff);
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif
        } while (++count < MAX_ITERATIONS && diff > MAX_ERROR);
        PIM_SREG[3] = (ulong_t)(diff * 1000000.0);
    }

#endif

