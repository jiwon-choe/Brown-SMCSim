#!/bin/bash

if [ $DRAM_PAGE_POLICY == "CLOSED" ]; then
	policy="close_page";
else
	policy="open_page";
fi

if [ $DRAMSIM2_ENABLE_DEBUG == "TRUE" ]; then
	debug="true";
else
	debug="false";
fi

echo -e "; COPY THIS FILE AND MODIFY IT TO SUIT YOUR NEEDS

NUM_CHANS=1							; This must be 1, because we instantiate it as a single channel in gem5
JEDEC_DATA_BUS_BITS=$DRAM_BUS_WIDTH 			 	; Always 64 for DDRx; if you want multiple *ganged* channels, set this to N*64
TRANS_QUEUE_DEPTH=$CHCTRL_W_FIFO_DEPTH				; transaction queue, i.e., CPU-level commands such as:  READ 0xbeef
CMD_QUEUE_DEPTH=$CHCTRL_W_FIFO_DEPTH				; command queue, i.e., DRAM-level commands such as: CAS 544, RAS 4
EPOCH_LENGTH=100000						; length of an epoch in cycles (granularity of simulation)
ROW_BUFFER_POLICY=$policy 					; close_page or open_page
ADDRESS_MAPPING_SCHEME=scheme2					;valid schemes 1-7; For multiple independent channels, use scheme7 since it has the most parallelism 
SCHEDULING_POLICY=rank_then_bank_round_robin 			; bank_then_rank_round_robin or rank_then_bank_round_robin 
QUEUING_STRUCTURE=per_rank					;per_rank or per_rank_per_bank

;for true/false, please use all lowercase
DEBUG_TRANS_Q=false
DEBUG_CMD_Q=false
DEBUG_ADDR_MAP=false
DEBUG_BUS=$debug
DEBUG_BANKSTATE=$debug
DEBUG_BANKS=false
DEBUG_POWER=false
VIS_FILE_OUTPUT=true

USE_LOW_POWER=true 						; go into low power mode when idle?
VERIFICATION_OUTPUT=false 					; should be false for normal operation
TOTAL_ROW_ACCESSES=4						; maximum number of open page requests to send to the same row before forcing a row close (to prevent starvation)"