#include "defs.hh"
#include "kernel_params.h"
#include <stdint.h>
#include <stdio.h>


/******************************************************************************/


typedef struct node {
    ulong_t key;      // 4 bytes
    struct node *next; // 4 bytes
} node_t;

typedef struct skiplist_node {
    ulong_t key;                    // 4 bytes
    struct skiplist_node *next[14]; // 14*4 bytes
    ulong_t top_level;              // 4 bytes
} skiplist_node_t;


#define BLOCK_SIZE sizeof(skiplist_node_t)
#define NUM_MEM_BLOCKS (0x8000000 / BLOCK_SIZE)
#define BLOCK_BASE_ADDR 0xC0000000 // for pim vault
#define REQUEST_SLOT_BASE_ADDR (request_slot_t *)(&PIM_VREG[0])
#define FREE_BLOCKS_HEAD PIM_SREG[3]
#define LINKED_LIST_HEAD PIM_SREG[4]
#define LINKED_LIST_TAIL PIM_SREG[5]



typedef struct free_block_node {
    struct free_block_node *next; // 4 bytes
} free_block_node_t;


node_t * alloc_memory_block() {
    // return address of allocated block; 0 if memory not available
    // if FREE_BLOCKS_HEAD == 0, memory is not available.
    node_t *mem_block = (node_t *)FREE_BLOCKS_HEAD;
    if (mem_block != 0) {
        FREE_BLOCKS_HEAD = (ulong_t)(((free_block_node_t *)mem_block)->next);
    }
    return mem_block;
}


void free_memory_block(node_t *nodeptr) {
    ((free_block_node_t *)nodeptr)->next = (free_block_node_t *)FREE_BLOCKS_HEAD;
    FREE_BLOCKS_HEAD = (ulong_t)nodeptr;
}


void list_init()
{
    int i = 0;
    free_block_node_t *free_block;

    pim_print_msg("inside list_init function");
    pim_print_hex("num_mem_blocks", NUM_MEM_BLOCKS);
    pim_print_hex("block_base_addr", BLOCK_BASE_ADDR);
 
    // initialize free blocks list   
    FREE_BLOCKS_HEAD = 0;
    for (i = 0; i < NUM_MEM_BLOCKS; i++) {
        free_block = (free_block_node_t *)(BLOCK_BASE_ADDR + (i*BLOCK_SIZE));
        /*if (((ulong_t)free_block) % 0x10000 == 0) {
            pim_print_hex("free block addr", free_block);
        }*/
        free_block->next = (free_block_node_t *)FREE_BLOCKS_HEAD;
        FREE_BLOCKS_HEAD = (ulong_t)free_block;
    }

    // initialize head/tail sentinel nodes
/*    LINKED_LIST_HEAD->key = 0;
    LINKED_LIST_HEAD->next = LINKED_LIST_TAIL;
    LINKED_LIST_TAIL->key = UINT32_MAX;
    LINKED_LIST_TAIL->next = (node_t *)0;*/
    LINKED_LIST_HEAD = (skiplist_node_t *)alloc_memory_block();
    LINKED_LIST_TAIL = (skiplist_node_t *)alloc_memory_block();
    ((skiplist_node_t *)LINKED_LIST_HEAD)->key = 0;
    ((skiplist_node_t *)LINKED_LIST_TAIL)->key = UINT32_MAX;
    ((skiplist_node_t *)LINKED_LIST_HEAD)->top_level = 14;
    ((skiplist_node_t *)LINKED_LIST_TAIL)->top_level = 14;
    for (i = 0; i < 14; i++) {
        ((skiplist_node_t *)LINKED_LIST_HEAD)->next[i] = (skiplist_node_t *)LINKED_LIST_TAIL;
        ((skiplist_node_t *)LINKED_LIST_TAIL)->next[i] = NULL;
    }

#ifndef kernel_default
    PIM_SREG[1] = NUM_MEM_BLOCKS;
    PIM_SREG[2] = 0;
#endif
}




void list_print()
{
    node_t *curr = (node_t *)LINKED_LIST_HEAD;
    do {
        curr = curr->next;
        pim_print_integer(curr->key);
    } while (curr->next != (node_t *)LINKED_LIST_TAIL);
}


#ifdef kernel_default
void execute_kernel
{
    // nothing -- used for compilation
}
#endif

#ifdef kernel_fc_with_sort_nonstop

int partition(request_slot_t **requests, int low_index, int high_index) 
{
    int pivot = (requests[low_index])->parameter;
    int i = low_index - 1;
    int j = high_index + 1;
    request_slot_t *swap = NULL;

    while (1) {
        do {
            i++; 
        } while ((requests[i])->parameter < pivot);
        do {
            j--;
        } while((requests[j])->parameter > pivot);

        if (i >= j) {
            return j;
        }

        swap = requests[i];
        requests[i] = requests[j];
        requests[j] = swap;
    }
}

void quicksort(request_slot_t **requests, int low_index, int high_index)
{
    int pivot = 0;

    if (low_index < high_index) {
        pivot = partition(requests, low_index, high_index);
        quicksort(requests, low_index, pivot);
        quicksort(requests, pivot+1, high_index);
    }
}


/*
    SREG[0]: specify PIM operation (0 == list initialization, 1 == linked list operation, 2 == print, 3 == add initial nodes, terminate otherwise)
    SREG[1]: PIM returns to HOST maximum key value allowed on PIM // HOST sends PIM max_level for initial list 
    SREG[2]: operation timestamp counter
    SREG[3]: free blocks list head
    SREG[4]: ptr to linked list sentinel head
    SREG[5]: ptr to linked list sentinel tail
    SREG[6]: 
    SREG[7]: 
*/
void execute_kernel()
{
    request_slot_t * request_slots = REQUEST_SLOT_BASE_ADDR;
    request_slot_t * valid_requests[NUM_HOST_THREADS] = {0}; // here, NUM_HOST_THREADS is equal to number of host threads making linkedlist requests (because there is no dedicated PIM operator thread)
    int i = 0;
    int num_valid_requests = 0;
    node_t *pred_node = (node_t *)LINKED_LIST_HEAD;
    node_t *curr_node = pred_node->next;
    node_t *new_node = NULL;
    uint16_t op_type = 0;
    ulong_t param = 0;
    uint16_t retval = 0;
    skiplist_node_t *preds[14] =  {0};
    skiplist_node_t *succs[14] = {0};
    skiplist_node_t *prev_skipnode = NULL;
    skiplist_node_t *found_skipnode = NULL;
    skiplist_node_t *curr_skipnode = NULL;
    skiplist_node_t *new_skipnode = NULL;
    int level;

    pim_print_msg("execute_kernel");

    while (true) {
        if (PIM_SREG[0] == 0) {
            // prepare kernel (init)
            list_init();
            PIM_SREG[0] = 3;
        } else if (PIM_SREG[0] == 2) {
            // print list
            list_print();
            PIM_SREG[0] = 1;
        } else if (PIM_SREG[0] == 3) {
            if (CHECK_VALID_REQUEST(request_slots[0].info_bits) != 0) {
                pim_print_hex("add", request_slots[0].parameter);
                // find where to insert node
                prev_skipnode = (skiplist_node_t *)LINKED_LIST_HEAD;
                found_skipnode = NULL;
                for (level = 13; level >= 0; level--) {
                    curr_skipnode = prev_skipnode->next[level];
                    while (request_slots[0].parameter > curr_skipnode->key) {
                        prev_skipnode = curr_skipnode;
                        curr_skipnode = prev_skipnode->next[level];
                    }
                    if ((found_skipnode == NULL) 
                        && (request_slots[0].parameter == curr_skipnode->key)) {
                        found_skipnode = curr_skipnode;
                        break;
                    }
                    preds[level] = prev_skipnode;
                    succs[level] = curr_skipnode;
                } // for each level
                pim_print_msg("iterated through each level");
                if (found_skipnode == NULL) {
                    new_skipnode = (skiplist_node_t *)alloc_memory_block();
                    new_skipnode->top_level = PIM_SREG[1];
                    new_skipnode->key = request_slots[0].parameter;
                    for (level = 0; level < new_skipnode->top_level; level++) {
                        new_skipnode->next[level] = succs[level];
                        preds[level]->next[level] = new_skipnode;
                    }
                    SET_RETVAL(request_slots[0].info_bits, 1);
                }
                CLEAR_REQUEST(request_slots[0].info_bits);
            } // if valid request
        } else if (PIM_SREG[0] >= 4) {
            // terminate
            break;
        } else {
            // linked list ops
            for (i = 0; i < 1000; i++) {
                // wait, TODO? how long should this wait be?
            } 

            // gather valid requests
            num_valid_requests = 0;
            for (i = 0; i < NUM_HOST_THREADS; i++) {
                if (CHECK_VALID_REQUEST(request_slots[i].info_bits) != 0) {
                    pim_print_hex("thread number", (ulong_t)i);
                    pim_print_hex("infobits", (ulong_t)(request_slots[i].info_bits));
                    pim_print_hex("parameter", (ulong_t)(request_slots[i].parameter));
                    valid_requests[num_valid_requests] = &(request_slots[i]);
                    num_valid_requests++;
                }
            }
            pim_print_hex("number of concurrent requests", num_valid_requests);

            // sort valid requests
            quicksort(valid_requests, 0, num_valid_requests-1);

            // execute sorted requests
            pred_node = (node_t *)LINKED_LIST_HEAD;
            curr_node = pred_node->next;
            for (i = 0; i < num_valid_requests; i++) {
                op_type = EXTRACT_OPERATION((valid_requests[i])->info_bits);
                param = (valid_requests[i])->parameter;
                retval = 0;

                while (curr_node->key < param) {
                    pred_node = curr_node;
                    curr_node = curr_node->next;
                }

                if (op_type == CONTAINS_OP) {
                    pim_print_msg("contains");
                    if (curr_node->key == param) {
                        retval = 1;
                    }
                } else if (op_type == ADD_OP) {
                    pim_print_msg("add");
                    if (curr_node->key != param) {
                        new_node = alloc_memory_block();
                        if (new_node != 0) {
                            new_node->key = param;
                            new_node->next = curr_node;
                            pred_node->next = new_node;
                            curr_node = new_node;
                            retval = 1;
                        }
                    }
                } else if (op_type == REMOVE_OP) {
                    pim_print_msg("remove");
                    if (curr_node->key == param) {
                        pred_node->next = curr_node->next;
                        free_memory_block(curr_node);
                        curr_node = pred_node->next;
                        retval = 1;
                    }
                }

#ifdef PIM_DEBUG_ON
                pim_print_msg("PIM_DEBUG_ON");
                (valid_requests[i])->timestamp = (uint16_t)PIM_SREG[2];
                PIM_SREG[2] = PIM_SREG[2] + 1;
#endif
                // set return value, clear request
                pim_print_hex("param", param);
                pim_print_hex("retval", retval);
                SET_RETVAL((valid_requests[i])->info_bits, retval);
                CLEAR_REQUEST((valid_requests[i])->info_bits);
            }
        }
    }
}
#endif
