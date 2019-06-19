#include "defs.hh"
#include "kernel_params.h"
#include <stdint.h>
#include <stdio.h>


/******************************************************************************/

//#define MAX_LEVEL 30

typedef struct node {
    ulong_t key;                    // 4 bytes
    ulong_t top_level;              // 4 bytes
    struct node *next[MAX_LEVEL]; // 30*4 bytes
} node_t; // 32*4 = 128 bytes


#define BLOCK_SIZE sizeof(node_t)
#define NUM_MEM_BLOCKS (0x8000000 / BLOCK_SIZE)
#define BLOCK_BASE_ADDR 0xC0000000 // for pim vault
#define REQUEST_SLOT_BASE_ADDR (request_slot_t *)(&PIM_VREG[0])
#define PRED_NODES_BASE_ADDR (node_t **)(&PIM_VREG[0] + (sizeof(request_slot_t) * NUM_HOST_THREADS))
#define SUCC_NODES_BASE_ADDR (node_t **)((void *)PRED_NODES_BASE_ADDR + (MAX_LEVEL * sizeof(node_t *)))
#define FREE_BLOCKS_HEAD PIM_SREG[3]
#define LIST_HEAD PIM_SREG[4]
#define LIST_TAIL PIM_SREG[5]
#define CURR_MAX_LEVEL PIM_SREG[6]



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
        free_block->next = (free_block_node_t *)FREE_BLOCKS_HEAD;
        FREE_BLOCKS_HEAD = (ulong_t)free_block;
    }

    // initialize head/tail sentinel nodes
    LIST_HEAD = (ulong_t)alloc_memory_block();
    LIST_TAIL = (ulong_t)alloc_memory_block();
    ((node_t *)LIST_HEAD)->key = 0;
    ((node_t *)LIST_TAIL)->key = UINT32_MAX;
    ((node_t *)LIST_HEAD)->top_level = MAX_LEVEL;
    ((node_t *)LIST_TAIL)->top_level = MAX_LEVEL;
    for (i = 0; i < MAX_LEVEL; i++) {
        ((node_t *)LIST_HEAD)->next[i] = (node_t *)LIST_TAIL;
        ((node_t *)LIST_TAIL)->next[i] = NULL;
    }

    PIM_SREG[1] = NUM_MEM_BLOCKS;
    PIM_SREG[2] = 0;
}

#ifdef kernel_default
/*
    SREG[0]: specify PIM operation (0 == list initialization, 1 == skiplist operation, 2 == print, terminate otherwise)
    SREG[1]: PIM returns to HOST maximum key value allowed on PIM // HOST sends PIM max_level for initial list 
    SREG[2]: operation timestamp counter
    SREG[3]: free blocks list head
    SREG[4]: ptr to skiplist sentinel head
    SREG[5]: ptr to skiplist sentinel tail
    SREG[6]: curr max level (max level for provided list size. since MAX_LEVEL is set to more than enough)
    SREG[7]: 
*/
void execute_kernel()
{
    request_slot_t *request_slots = REQUEST_SLOT_BASE_ADDR;
    int i = 0;
    uint8_t valid = 0;
    uint8_t requested_op = 0;
    node_t *prev_node = NULL;
    node_t *found_node = NULL;
    node_t *curr_node = NULL;
    node_t *new_node = NULL;
    node_t **preds = PRED_NODES_BASE_ADDR;
    node_t **succs = SUCC_NODES_BASE_ADDR;
    int level = 0;
    uint8_t retval = 0;

    pim_print_msg("execute_kernel");
    pim_print_hex("size of node_t", sizeof(node_t));
    pim_print_hex("REQUEST_SLOT_BASE_ADDR", (ulong_t)REQUEST_SLOT_BASE_ADDR);
    pim_print_hex("size of request_slot_t", sizeof(request_slot_t));
    pim_print_hex("sizeof(node_t *)", sizeof(node_t *));
    pim_print_hex("PRED_NODES ADDR", (ulong_t)preds);
    pim_print_hex("SUCC_NODES ADDR", (ulong_t)succs);
    while (true) {
        if (PIM_SREG[0] == 0) {
            pim_print_msg("list init");
            list_init();
            PIM_SREG[0] = 1;
        } else if (PIM_SREG[0] >= 3) {
            pim_print_msg("close pim");
            break;
        } else { // if PIM_SREG[0] == 1
            //pim_print_msg("skiplist ops");
            for (i = 0; i < NUM_HOST_THREADS; i++) {
                valid = CHECK_VALID_REQUEST(request_slots[i].info_bits);
                if (valid == 0) {
                    continue;
                }
                requested_op = EXTRACT_OPERATION(request_slots[i].info_bits);

                //find node
                prev_node = (node_t *)LIST_HEAD;
                found_node = NULL;
                retval = 0;
                for (level = CURR_MAX_LEVEL-1; level >= 0; level--) { // level must be SIGNED int for this to terminate!!
                    //pim_print_hex("at level", level);
                    curr_node = prev_node->next[level];
                    while (request_slots[i].parameter > curr_node->key) {
                        prev_node = curr_node; 
                        curr_node = prev_node->next[level];
                    }
                    if ((found_node == NULL) && (request_slots[i].parameter == curr_node->key)) {
                        found_node = curr_node;
                        if (requested_op != REMOVE_OP) {
                            break; // if node already exists, retval = 1 for CONTAINS, = 0 for ADD
                        }
                    }
                    preds[level] = prev_node;
                    succs[level] = curr_node;
                    //pim_print_hex("preds key", prev_node->key);
                    //pim_print_hex("succs key", curr_node->key);
                } // for each level
                // execute
                if (requested_op == CONTAINS_OP) {
                    if (found_node != NULL) {
                        retval = 1;
                    }
                    pim_print_hex("contains", request_slots[i].parameter);
                } else if (requested_op == ADD_OP) {
                    if (found_node == NULL) {
                        retval = 1;
                        new_node = alloc_memory_block();
                        new_node->top_level = request_slots[i].random_level;
                        new_node->key = request_slots[i].parameter;
                        for (level = 0; level < new_node->top_level; level++) {
                            new_node->next[level] = succs[level];
                            preds[level]->next[level] = new_node;
                        }
                        pim_print_hex("add", request_slots[i].parameter);
                    }
                } else { // REMOVE_OP
                    if (found_node != NULL) {
                        retval = 1;
                        for (level = found_node->top_level-1; level >= 0; level--) {
                            preds[level]->next[level] = found_node->next[level];
                        }
                    }
                    pim_print_hex("remove", request_slots[i].parameter);
                }

                pim_print_hex("retval", retval);
                #ifdef PIM_DEBUG_ON
                request_slots[i].timestamp = (uint16_t)PIM_SREG[2];
                PIM_SREG[2] = PIM_SREG[2] + 1;
                #endif
                SET_RETVAL(request_slots[i].info_bits, retval);
                CLEAR_REQUEST(request_slots[i].info_bits);
            } // for i in NUM_HOST_THREADS
        } // if PIM_SREG[0]
    } // while true
}
#endif

#ifdef kernel_reduce_redundancy
void execute_kernel()
{
    request_slot_t *request_slots = REQUEST_SLOT_BASE_ADDR;
    int i = 0;
    uint8_t valid = 0;
    uint8_t requested_op = 0;
    node_t *prev_node = NULL;
    node_t *found_node = NULL;
    node_t *curr_node = NULL;
    node_t *new_node = NULL;
    node_t **preds = PRED_NODES_BASE_ADDR;
    node_t **succs = SUCC_NODES_BASE_ADDR;
    node_t *next_node = NULL;
    int level = 0;
    int lowest_valid_level = 0;
    uint8_t retval = 0;

    while (true) {
        if (PIM_SREG[0] == 0) {
            pim_print_msg("list init");
            list_init();
            PIM_SREG[0] = 1;
        } else if (PIM_SREG[0] >= 3) {
            pim_print_msg("close pim");
            break;
        } else { // if PIM_SREG[0] == 1
            for (i = 0; i < NUM_HOST_THREADS; i++) {
                valid = CHECK_VALID_REQUEST(request_slots[i].info_bits);
                if (valid == 0) {
                    continue;
                }
                requested_op = EXTRACT_OPERATION(request_slots[i].info_bits);

                //find node
                prev_node = (node_t *)LIST_HEAD;
                found_node = NULL;
                retval = 0;
                lowest_valid_level = 0;
                for (level = CURR_MAX_LEVEL-1; level >= 0; level--) { // level must be SIGNED int for this to terminate!!
                    curr_node = prev_node->next[level];
                    if (curr_node == NULL) {
                        // all ptrs from this level and below are redundant (same as level+1)
                        lowest_valid_level = level+1;
                        break;
                    }
                    while (request_slots[i].parameter > curr_node->key) {
                        prev_node = curr_node; 
                        curr_node = prev_node->next[level];
                    }
                    if ((found_node == NULL) && (request_slots[i].parameter == curr_node->key)) {
                        found_node = curr_node;
                        if (requested_op != REMOVE_OP) {
                            break; // if node already exists, retval = 1 for CONTAINS, = 0 for ADD
                        }
                    }
                    preds[level] = prev_node;
                    succs[level] = curr_node;
                    //pim_print_hex("preds key", prev_node->key);
                    //pim_print_hex("succs key", curr_node->key);
                } // for each level
                // execute
                if (requested_op == CONTAINS_OP) {
                    if (found_node != NULL) {
                        retval = 1;
                    }
                    pim_print_hex("contains", request_slots[i].parameter);
                } else if (requested_op == ADD_OP) {
                    if (found_node == NULL) {
                        retval = 1;
                        new_node = alloc_memory_block();
                        new_node->top_level = request_slots[i].random_level;
                        new_node->key = request_slots[i].parameter;
                        if (lowest_valid_level == 0) {
                            for (level = 0; level < new_node->top_level; level++) {
                                new_node->next[level] = succs[level];
                                preds[level]->next[level] = new_node;
                            }
                        } else {
                            for (level = lowest_valid_level-1; level >= 0; level--) {
                                preds[level] = preds[lowest_valid_level];
                                succs[level] = succs[lowest_valid_level];
                            }
                            // connect higher levels
                            for (level = new_node->top_level; level < lowest_valid_level; level++) {
                                preds[level]->next[level] = succs[level];
                            }
                            // connect new node
                            for (level = 0; level < new_node->top_level; level++) {
                                new_node->next[level] = succs[level];
                                preds[level]->next[level] = new_node;
                            }
                        }

                        pim_print_hex("add", request_slots[i].parameter);
                    }
                } else { // REMOVE_OP
                    if (found_node != NULL) {
                        retval = 1;
#if 0
                        for (level = found_node->top_level-1; level >= 0; level--) {
                            preds[level]->next[level] = found_node->next[level];
                        }
#endif
                        for (level = lowest_valid_level-1; level >= 0; level--) {
                            preds[level] = preds[lowest_valid_level];
                            succs[level] = succs[lowest_valid_level];
                        }
                        for (level = found_node->top_level-1; level >= 0; level--) {
                            // at this point, succs[level] == found_node
                            if (found_node->next[level] != NULL) {
                                succs[level] = found_node->next[level];
                            } else {
                                succs[level] = NULL;
                            }
                        }
                    }
                    pim_print_hex("remove", request_slots[i].parameter);
                }

                pim_print_hex("retval", retval);
                #ifdef PIM_DEBUG_ON
                request_slots[i].timestamp = (uint16_t)PIM_SREG[2];
                PIM_SREG[2] = PIM_SREG[2] + 1;
                #endif
                SET_RETVAL(request_slots[i].info_bits, retval);
                CLEAR_REQUEST(request_slots[i].info_bits);
            } // for i in NUM_HOST_THREADS
        } // if PIM_SREG[0]
    } // while true
}
#endif 
