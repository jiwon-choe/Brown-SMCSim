
#include "defs.hh"
#include "kernel_params.h"

#define ELEMENT(M, S, r, c) M[r*S+c]

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        unsigned i, j;
        ulong_t NODES, MAX_ITERATIONS, count;
        node* nodes;
        ulong_t* adj;
        float diff, delta, f, MAX_ERROR;
        float* pf;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: adj matrix
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */

        nodes = (node*)PIM_SREG[0];
        adj = (ulong_t*)PIM_SREG[1];
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
                for ( j=0; j<NODES; j++ ) // for node.successors
                    if ( ELEMENT(adj, NODES, i, j) != NC ) // if j is a successor
                        nodes[j].next_rank += delta;
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
#ifdef kernel_coprocessor

    Notice: This code does not work, because it assumes that float_t types are ulong_t
        It must be modified before use

    #define CONST__1_00 0x3f800000    // 1.00 (float)
    #define CONST__0_15 0x3e19999a    // 0.15 (float)
    #define CONST__0_85 0x3f59999a    // 0.85 (float)
    #define CONST__0_00 0x00000000    // 0.00 (float)

    #define ELEMENT(M, S, r, c) M[r*S+c]

    void execute_kernel()
    {
        unsigned r, c;
        ulong_t NODES, MAX_ERROR, MAX_ITERATIONS, NODES_F, count, diff, delta;
        node* nodes;
        ulong_t* adj;
        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: adj matrix
        SREG[2]: number of nodes
        SREG[3]: retval (output from PIM)
        SREG[4]: MAXIMUM ERROR
        SREG[5]: MAXIMUM ITERATIONS
        */
        nodes = (node*)PIM_SREG[0];
        adj = (ulong_t*)PIM_SREG[1];
        NODES = PIM_SREG[2];
        MAX_ERROR = PIM_SREG[4];
        MAX_ITERATIONS = PIM_SREG[5];

        pim_print_hex("NODES", NODES);
        pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
        pim_print_hex("MAX ERROR (float)", MAX_ERROR);

        /* NODES_F = (float) NODES */
        PIM_SREG[0] = NODES;
        PIM_COPROCESSOR_CMD = CMD_TO_FLOAT;     // S1 = float(S0)
        NODES_F = PIM_SREG[1];
        for ( r=0; r<NODES; r++ )
        {
            /* nodes[r].page_rank = 1.0 / NODES; */
            PIM_SREG[0] = CONST__1_00;
            PIM_SREG[1] = NODES_F;
            PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
            nodes[r].page_rank = PIM_SREG[2];
            
            /* nodes[r].next_rank = 0.15 / NODES; */
            PIM_SREG[0] = CONST__0_15;
            PIM_SREG[1] = NODES_F;
            PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
            nodes[r].next_rank = PIM_SREG[2];
        }
        count = 0;

        diff = CONST__0_00;
        do {
            for ( r=0; r<NODES; r++ )
            {
                /* float delta = 0.85 * nodes[r].page_rank / nodes[r].out_degree; */
                PIM_SREG[0] = CONST__0_85;
                PIM_SREG[1] = nodes[r].page_rank;
                PIM_COPROCESSOR_CMD = CMD_FMUL;     // S2 = S0 * S1
                PIM_SREG[0] = PIM_SREG[2];
                PIM_SREG[1] = nodes[r].out_degree;
                PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
                delta = PIM_SREG[2];

                for ( c=0; c<NODES; c++ ) // for node.successors
                    if ( ELEMENT(adj, NODES, r, c) != NC ) // if c is a successor
                    {
                        /* nodes[c].next_rank += delta; */
                        PIM_SREG[0] = delta;
                        PIM_SREG[1] = nodes[c].next_rank;
                        PIM_COPROCESSOR_CMD = CMD_FACC;     // S1 = S0 + S1
                        nodes[c].next_rank = PIM_SREG[1];
                    }
            }
            diff = CONST__0_00;
            for ( r=0; r<NODES; r++ )
            {
                /* diff += fabsf(nodes[r].next_rank - nodes[r].page_rank); */
                PIM_SREG[0] = nodes[r].next_rank;
                PIM_SREG[1] = nodes[r].page_rank;
                PIM_COPROCESSOR_CMD = CMD_FSUB;     // S2 = S0 - S1
                PIM_SREG[0] = PIM_SREG[2];
                PIM_COPROCESSOR_CMD = CMD_FABS;     // S1 = fabsf(S0)
                PIM_SREG[0] = diff;
                PIM_COPROCESSOR_CMD = CMD_FACC;     // S1 = S0 + S1
                diff = PIM_SREG[1];

                nodes[r].page_rank = nodes[r].next_rank;

                /* nodes[r].next_rank = 0.15 / NODES; */
                PIM_SREG[0] = CONST__0_15;
                PIM_SREG[1] = NODES_F;
                PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
                nodes[r].next_rank = PIM_SREG[2];
            }
            pim_print_hex("iteration", count);
            #ifdef DEBUG_RESIDENT
            PIM_SREG[0] = diff;
            PIM_COPROCESSOR_CMD = CMD_FPRINT;
            #endif

            /* diff > e */
            PIM_SREG[0] = diff;
            PIM_SREG[1] = MAX_ERROR;
            PIM_COPROCESSOR_CMD = CMD_FCMP; // S2 = (S0>S1)?1.0:0
        } while (++count < MAX_ITERATIONS && PIM_SREG[2] != 0 );

        /* return (ulong_t)(diff * 1000000.0); */
        PIM_SREG[0] = diff;
        PIM_COPROCESSOR_CMD = CMD_TO_FIXED;
        PIM_SREG[3] = PIM_SREG[1]; // Return
    }

#endif

/******************************************************************************/
#ifdef kernel_fpu_coproc

/* This is not necessary, because NEON is already available on PIM*/

#define CONST__1_00 0x3f800000    // 1.00 (float)
#define CONST__0_15 0x3e19999a    // 0.15 (float)
#define CONST__0_85 0x3f59999a    // 0.85 (float)
#define CONST__0_00 0x00000000    // 0.00 (float)

#define ELEMENT(M, S, r, c) M[r*S+c]

void execute_kernel()
{
    unsigned r, c;
    ulong_t NODES, MAX_ERROR, MAX_ITERATIONS, NODES_F, count, diff, delta;
    node* nodes;
    ulong_t* adj;
    /* PIM Scalar Registers
    SREG[0]: nodes
    SREG[1]: adj matrix
    SREG[2]: number of nodes
    SREG[3]: retval (output from PIM)
    SREG[4]: MAXIMUM ERROR
    SREG[5]: MAXIMUM ITERATIONS
    */
    nodes = (node*)PIM_SREG[0];
    adj = (ulong_t*)PIM_SREG[1];
    NODES = PIM_SREG[2];
    MAX_ERROR = PIM_SREG[4];
    MAX_ITERATIONS = PIM_SREG[5];

    pim_print_hex("NODES", NODES);
    pim_print_hex("MAX ITERATIONS", MAX_ITERATIONS);
    pim_print_hex("MAX ERROR (float)", MAX_ERROR);

    /* NODES_F = (float) NODES */
    PIM_SREG[0] = NODES;
    PIM_COPROCESSOR_CMD = CMD_TO_FLOAT;     // S1 = float(S0)
    NODES_F = PIM_SREG[1];
    for ( r=0; r<NODES; r++ )
    {
        /* nodes[r].page_rank = 1.0 / NODES; */
        PIM_SREG[0] = CONST__1_00;
        PIM_SREG[1] = NODES_F;
        PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
        nodes[r].page_rank = PIM_SREG[2];
        
        /* nodes[r].next_rank = 0.15 / NODES; */
        PIM_SREG[0] = CONST__0_15;
        PIM_SREG[1] = NODES_F;
        PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
        nodes[r].next_rank = PIM_SREG[2];
    }
    count = 0;

    diff = CONST__0_00;
    do {
        for ( r=0; r<NODES; r++ )
        {
            /* float delta = 0.85 * nodes[r].page_rank / nodes[r].out_degree; */
            PIM_SREG[0] = CONST__0_85;
            PIM_SREG[1] = nodes[r].page_rank;
            PIM_COPROCESSOR_CMD = CMD_FMUL;     // S2 = S0 * S1
            PIM_SREG[0] = PIM_SREG[2];
            PIM_SREG[1] = nodes[r].out_degree;
            PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
            delta = PIM_SREG[2];

            for ( c=0; c<NODES; c++ ) // for node.successors
                if ( ELEMENT(adj, NODES, r, c) != NC ) // if c is a successor
                {
                    /* nodes[c].next_rank += delta; */
                    PIM_SREG[0] = delta;
                    PIM_SREG[1] = nodes[c].next_rank;
                    PIM_COPROCESSOR_CMD = CMD_FACC;     // S1 = S0 + S1
                    nodes[c].next_rank = PIM_SREG[1];
                }
        }
        diff = CONST__0_00;
        for ( r=0; r<NODES; r++ )
        {
            /* diff += fabsf(nodes[r].next_rank - nodes[r].page_rank); */
            PIM_SREG[0] = nodes[r].next_rank;
            PIM_SREG[1] = nodes[r].page_rank;
            PIM_COPROCESSOR_CMD = CMD_FSUB;     // S2 = S0 - S1
            PIM_SREG[0] = PIM_SREG[2];
            PIM_COPROCESSOR_CMD = CMD_FABS;     // S1 = fabsf(S0)
            PIM_SREG[0] = diff;
            PIM_COPROCESSOR_CMD = CMD_FACC;     // S1 = S0 + S1
            diff = PIM_SREG[1];

            nodes[r].page_rank = nodes[r].next_rank;

            /* nodes[r].next_rank = 0.15 / NODES; */
            PIM_SREG[0] = CONST__0_15;
            PIM_SREG[1] = NODES_F;
            PIM_COPROCESSOR_CMD = CMD_FDIV;     // S2 = S0 / S1
            nodes[r].next_rank = PIM_SREG[2];
        }
        pim_print_hex("iteration", count);
        #ifdef DEBUG_RESIDENT
        PIM_SREG[0] = diff;
        PIM_COPROCESSOR_CMD = CMD_FPRINT;
        #endif

        /* diff > e */
        PIM_SREG[0] = diff;
        PIM_SREG[1] = MAX_ERROR;
        PIM_COPROCESSOR_CMD = CMD_FCMP; // S2 = (S0>S1)?1.0:0
    } while (++count < MAX_ITERATIONS && PIM_SREG[2] != 0 );

    /* return (ulong_t)(diff * 1000000.0); */
    PIM_SREG[0] = diff;
    PIM_COPROCESSOR_CMD = CMD_TO_FIXED;
    PIM_SREG[3] = PIM_SREG[1]; // Return
}

#endif