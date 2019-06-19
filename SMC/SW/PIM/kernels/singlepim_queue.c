#include "defs.hh"
#include "kernel_params.h"
#include <stdint.h>
#include <stdio.h>


/******************************************************************************/

#define MAX_SEGMENT_BYTES PIM_SREG[1] 
#define MAX_SEGMENT_ITEMS (MAX_SEGMENT_BYTES / sizeof(ulong_t))

#define NUM_MEM_BLOCKS (0x8000000 / MAX_SEGMENT_BYTES)
#define BLOCK_BASE_ADDR 0xC0000000 // pim vault

#define REQUEST_SLOT_BASE_ADDR (request_slot_t *)(&PIM_VREG[0])

#define CURR_SEGMENT ((ulong_t *)PIM_SREG[3])
#define HEAD_INDEX PIM_SREG[4]
#define TAIL_INDEX PIM_SREG[5]
#define MAX_INDEX 33554432 //(128*1024*1024 bytes / 4 bytes/node)



/*
PIM_SREG[0]: specify PIM operation (0 == OFF, 
                                    1 == ON for ENQ, 
                                    2 == ON for DEQ,
                                    3 == INIT,
                                    4 == PRINT)
PIM_SREG[1]: MAX_SEGMENT_BYTES
PIM_SREG[2]: 
PIM_SREG[3]: ptr to currently active segment
PIM_SREG[4]: curr head index
PIM_SREG[5]: curr tail index
PIM_SREG[6]: timestamp
PIM_SREG[7]: 
*/

void execute_kernel()
{
    request_slot_t *request_slots = REQUEST_SLOT_BASE_ADDR;
    int i = 0;
    uint16_t valid = 0;

    pim_print_hex("exec kernel with op", PIM_SREG[0]);

    while (true) {
        if (PIM_SREG[0] == 3) { // INIT
            pim_print_msg("init!");
            PIM_SREG[3] = BLOCK_BASE_ADDR;
            HEAD_INDEX = 0;
            TAIL_INDEX = 0;
            PIM_SREG[0] = 0;
            PIM_SREG[6] = 0; // initialize timestamp

        } else if (PIM_SREG[0] == 1) { // ENQ/DEQ
            for (i = 0; i < NUM_HOST_THREADS; i++) {
                valid = CHECK_VALID_REQUEST(request_slots[i].info_bits);
                if (valid == 0) {
                    continue;
                }

                if (EXTRACT_OP(request_slots[i].info_bits) == 0) {
                    // DEQ
                    request_slots[i].parameter = CURR_SEGMENT[HEAD_INDEX];
                    pim_print_hex("parameter", request_slots[i].parameter);
#ifdef PIM_DEBUG_ON
                    request_slots[i].timestamp = PIM_SREG[6];
                    PIM_SREG[6]++;
#endif
                    HEAD_INDEX++;
                    if (HEAD_INDEX >= MAX_INDEX) { // wrap around; possible since we assume that there is enough but not too big buffer between head and tail
                        HEAD_INDEX = 0;
                    }

                } else {
                    // ENQ
                    CURR_SEGMENT[TAIL_INDEX] = request_slots[i].parameter;
#ifdef PIM_DEBUG_ON
                    request_slots[i].timestamp = PIM_SREG[6];
                    PIM_SREG[6]++;
#endif
                    TAIL_INDEX++;
                    if (TAIL_INDEX >= MAX_INDEX) {
                        TAIL_INDEX = 0;
                    }
                }

                CLEAR_REQUEST(request_slots[i].info_bits);
            } // for each host thread

        } else if (PIM_SREG[0] == 4) { // PRINT
            for (i = HEAD_INDEX; i < TAIL_INDEX; i++) {
                pim_print_integer(CURR_SEGMENT[i]);
            }
            PIM_SREG[0] = 0;

        } else if (PIM_SREG[0] == 0) {
            // clearing the request should be done at the HOST side:
            //          -- before moving operation to next PIM, 
            //             the request slots set for this PIM should be cleared out
            //             so that future operations are not affected.
            break;
        }
    } // while true
    pim_print_msg("completed exec kernel");
}
