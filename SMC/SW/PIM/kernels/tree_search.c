
#include "defs.hh"
#include "kernel_params.h"

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
    	/* PIM Scalar Registers
    	SREG[0]: Tree
    	SREG[1]: Search List
        SREG[2]: Number of searches
        SREG[3]: Checksum
    	*/
    	node* tree;
    	ulong_t* search_list;
    	ulong_t num_operations, key, goal;
        ulong_t checksum = 0;
    	unsigned i;
        node* curr;

    	tree = (node*) PIM_SREG[0];
    	search_list = (ulong_t*) PIM_SREG[1];
    	num_operations = (ulong_t) PIM_SREG[2];

    	pim_print_msg("<<< BST SEARCH KERNEL >>>");
    	pim_print_hex("Tree", (ulong_t) tree );
    	pim_print_hex("Search List", (ulong_t) search_list );
    	pim_print_hex("Num Operations", num_operations);

        //DTLB_REFILL_ideal(tree);

        PIM_SREG[3] = 1;
        for (i=0; i<num_operations; i++ )
        {
            curr = tree;
            //DTLB_REFILL_ideal(&search_list[i]);
            goal = search_list[i];
            while (curr)
            {
                checksum++;
                //DTLB_REFILL_ideal(curr)
                //DTLB_REFILL_ideal(curr->left);
                //DTLB_REFILL_ideal(curr->right);

            	key = curr-> key;
                if ( key == goal )
                {
    				pim_print_hex("Found key", key );
                    break;
                }
                else
                if ( key > goal )
                    curr = (node*)(curr->left);
                else
                    curr = (node*)(curr->right);
            }
            if ( !curr )
            	pim_error("Error: node was not found!");
        }
        PIM_SREG[3] = checksum;
    //	pim_print_msg("Tree search finished ...");
    }

#endif

/******************************************************************************/
#ifdef kernel_dma

    void execute_kernel()
    {
        /* PIM Scalar Registers
        SREG[0]: Tree
        SREG[1]: Search List
        SREG[2]: Number of searches
        SREG[3]: Checksum
        */
        node* tree;
        ulong_t* search_list;
        ulong_t num_operations, key, goal;
        ulong_t checksum = 0;
        unsigned i;
        node* curr;

        tree = (node*) PIM_SREG[0];
        search_list = (ulong_t*) PIM_SREG[1];
        num_operations = (ulong_t) PIM_SREG[2];

        pim_print_msg("<<< BST SEARCH KERNEL >>>");
        pim_print_hex("Tree", (ulong_t) tree );
        pim_print_hex("Search List", (ulong_t) search_list );
        pim_print_hex("Num Operations", num_operations);

        PIM_SREG[3] = 1;
        for (i=0; i<num_operations; i++ )
        {
            curr = tree;
            goal = search_list[i];
            while (curr)
            {
                DMA_REQUEST(curr, &PIM_VREG[0], 3*sizeof(ulong_t), PIM_DMA_READ, DMA_RES0 );
                DMA_WAIT( DMA_RES0);
                curr = (node*) &PIM_VREG[0];
                checksum++;
                key = curr-> key;
                if ( key == goal )
                {
                    pim_print_hex("Found key", key );
                    break;
                }
                else
                if ( key > goal )
                    curr = (node*)(curr->left);
                else
                    curr = (node*)(curr->right);
            }
            if ( !curr )
                pim_error("Error: node was not found!");
        }
        PIM_SREG[3] = checksum;
    //  pim_print_msg("Tree search finished ...");
    }

#endif
