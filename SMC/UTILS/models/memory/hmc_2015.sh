#!/bin/bash

export N_TARG_PORT=8
export N_INIT_PORT=32
export AXI_DATA_W=256
export AXI_BURST_LENGTH=8
export N_MEM_DIES=4

export DRAM_BANK_PARALLELISM=TRUE
export HALT_ON_FUNCTIONAL_ERROR=FALSE
export DRAM_PAGE_POLICY="CLOSED";# "OPEN", "CLOSED"
export DRAM_BUS_WIDTH="32";	# bit  
export DRAM_BANKS_PER_DIE="2";		# banks
export DRAM_column_size="6";	# bit  
export DRAM_row_size="14";	# bit  
export DRAM_tCL="13.75";		# ns
export DRAM_tCK="0.8";		# ns   
export DRAM_tMRD="8.3";		# ns
export DRAM_tRTP="7.5";	# ns
export DRAM_tRAS="27.5";		# ns   
export DRAM_tRAP="5.6";		# ns   
export DRAM_tRCD="13.75";	# ns   
export DRAM_tRFC="84.6";		# ns   
export DRAM_tRP="13.75";		# ns   
export DRAM_tRRD="2.8";		# ns
export DRAM_tWR="15";		# ns
export DRAM_tAA="4.9";		# ns
export DRAM_tDLL_OFF="2.8";	# ns
export DRAM_tINIT0="20";			# ns - Apply Voltage RAMP - MAX
export DRAM_tINIT1="10";			# ns - Activate CLK (after tINIT0) for 100ns while cke is still Low  - MIN
export DRAM_tINIT3="40";			# ns - Wait tINIT3 after CKE before going in PRE_ALL
export DRAM_tREF_PERIOD="64000000";	# ns

P=(RC LB CH OF); export_array ADDRESS_MAPPING P


