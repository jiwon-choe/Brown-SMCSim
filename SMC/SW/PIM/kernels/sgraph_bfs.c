
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
        ulong_t c;
        ulong_t NODES;
        node* nodes;
        ulong_t* queue;     // queue for BFS search
        ulong_t  head, tail, elements;
        ulong_t total_distance;
        node* succ;
        unsigned MAX_ITERATIONS, x;
        node* v;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        SREG[4]: queue
        SREG[5]: MAX_ITERATIONS
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        MAX_ITERATIONS = PIM_SREG[5];
        queue = (ulong_t*)PIM_SREG[4];

        total_distance = 0;
        for (x=0; x<MAX_ITERATIONS; x++)
        {
            nodes[x].distance = 0;
            queue_init;
            pim_print_hex("Iteration", x);
            queue_push(&nodes[x]);               // Push the first element
            
            while ( !(queue_empty) )
            {
                v = (node*)queue_top;
                queue_pop;
                for ( c=0; c<v->out_degree ; c++ )
                {
                    succ = v->successors[c];
                    if (succ->distance == NC) // Infinite
                    {
                        succ->distance = v->distance + 1;
                        total_distance += succ->distance;
                        // pim_print_hex("visited node", succ->ID);
                        queue_push(succ);
                    }
                }
            }
        }

        pim_print_hex("total_distance", total_distance);
        PIM_SREG[3] = total_distance;
    }

#endif


/******************************************************************************/
#ifdef kernel_best

    void execute_kernel()
    {
        ulong_t c;
        ulong_t NODES;
        node* nodes;
        ulong_t* queue;     // queue for BFS search
        ulong_t  head, tail, elements;
        ulong_t total_distance;
        volatile node* v;
        volatile node** succ_ping;
        volatile node** succ_pong;
        volatile node** succ_swap;
        int num_pongs;
        unsigned curr_distance;
        unsigned MAX_ITERATIONS, x;

        /* PIM Scalar Registers
        SREG[0]: nodes
        SREG[1]: ---
        SREG[2]: number of nodes
        SREG[3]: retval
        SREG[4]: queue
        SREG[5]: MAX_ITERATIONS
        */
        nodes = (node*)PIM_SREG[0];
        NODES = PIM_SREG[2];
        ///queue = (ulong_t*)PIM_SREG[4];       // We allocate the Queue in the SPM
        MAX_ITERATIONS = PIM_SREG[5];
        queue = (ulong_t*)&PIM_VREG[0];        // NOTICE: Maximum Queue Size = 2 x XFER_SIZE
        succ_ping = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*2);
        succ_pong = (volatile node**)(&PIM_VREG[0] + MAX_XFER_SIZE*3);

        pim_print_hex("NODES", NODES);
        pim_print_hex("sizeof(node)", sizeof(node));
        pim_print_hex("nodes_chunk", nodes_chunk);

        total_distance = 0;
        for (x=0; x<MAX_ITERATIONS; x++)
        {
            nodes[x].distance = 0;
            queue_init;
            pim_print_hex("Iteration", x);
            queue_push(&nodes[x]);               // Push the first element

            curr_distance = 0;
            num_pongs = -1;
            while ( !(queue_empty) )
            {
                pim_assert(elements < 2*MAX_XFER_SIZE, "queue size exceeded SPM size!");
                v = (node*)queue_top;
                queue_pop;

                if ( v != NULL && v->out_degree )
                {
                    DMA_REQUEST(v->successors, succ_pong, v->out_degree * sizeof(node*), PIM_DMA_READ, DMA_RES1 );
                }


            MY_LABEL:
                if ( num_pongs != -1 )
                {
                    for ( c=0; c< num_pongs; c++ )
                    {
                        if (succ_ping[c]->distance == NC) // Infinite
                        {
                            succ_ping[c]->distance = curr_distance + 1;
                            total_distance += succ_ping[c]->distance;
                            //pim_print_hex("visited node", succ_ping[c]->ID);
                            queue_push(succ_ping[c]);
                        }
                    }
                    num_pongs = -1;
                }

                if ( v != NULL && v->out_degree )
                {
                    num_pongs = v->out_degree;
                    curr_distance = v->distance;
                    DMA_WAIT(DMA_RES1);
                    succ_swap = succ_ping; succ_ping = succ_pong; succ_pong = succ_swap;
                }
            }
            if ( num_pongs != -1 ){
                v = NULL;
                goto MY_LABEL; // Termination condition
            }
        }

        pim_print_hex("total_distance", total_distance);
        PIM_SREG[3] = total_distance;
    }

#endif

// /******************************************************************************/
// #ifdef kernel_simpledma

//     void execute_kernel()
//     {
//         ulong_t c;
//         ulong_t NODES;
//         node* nodes;
//         ulong_t* queue;     // queue for BFS search
//         ulong_t  head, tail, elements;
//         ulong_t total_distance;
//         volatile node* v;
//         volatile node** succ_ping;

//         /* PIM Scalar Registers
//         SREG[0]: nodes
//         SREG[1]: ---
//         SREG[2]: number of nodes
//         SREG[3]: retval
//         SREG[4]: queue
//         */
//         nodes = (node*)PIM_SREG[0];
//         NODES = PIM_SREG[2];
//         ///queue = (ulong_t*)PIM_SREG[4];
//         queue = (ulong_t*)&PIM_VREG[0];        // NOTICE: Maximum Queue Size = 2 x XFER_SIZE
//         succ_ping = (volatile node**)(&PIM_VREG[0] + XFER_SIZE*2);
//         //succ_pong = (volatile node**)(&PIM_VREG[0] + XFER_SIZE*3);
//         queue_init;
//         queue_push(&nodes[0]);               // Push the first element
//         pim_print_hex("NODES", NODES);
//         pim_print_hex("sizeof(node)", sizeof(node));
//         pim_print_hex("nodes_chunk", nodes_chunk);

//         // // Initialization phase
//         // DMA_REQUEST(nodes, nodes_pong, nodes_chunk, PIM_DMA_READ, DMA_RES0 );

//         total_distance = 0;
//         while ( !(queue_empty) )
//         {
//             v = (node*)queue_top;
//             queue_pop;

//             if ( v->out_degree )
//             {
//                 DMA_REQUEST(v->successors, succ_ping, v->out_degree * sizeof(node*), PIM_DMA_READ, DMA_RES1 );
//                 DMA_WAIT(DMA_RES1);

//                 for ( c=0; c< v->out_degree; c++ )
//                 {
//                     if (succ_ping[c]->distance == NC) // Infinite
//                     {
//                         succ_ping[c]->distance = v->distance + 1;
//                         total_distance += succ_ping[c]->distance;
//                         // pim_print_hex("visited node", succ->ID);
//                         queue_push(succ_ping[c]);
//                     }
//                 }
//             }
//         }

//         pim_print_hex("total_distance", total_distance);
//         PIM_SREG[3] = total_distance;
//     }

// #endif
