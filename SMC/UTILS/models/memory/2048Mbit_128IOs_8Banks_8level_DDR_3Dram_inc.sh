#!/bin/bash
export DRAM_PAGE_POLICY="OPEN";	# "OPEN", "CLOSED"
export DRAM_BUS_WIDTH="128";	# bit  
export DRAM_BANKS_PER_DIE="8";		# banks
export DRAM_column_size="8";	# bit  
export DRAM_row_size="13";	# bit  	# 12, 13, 14, 15, ...
export DRAM_tCL="8.4";		# ns
export DRAM_tCK="2.8";		# ns   
export DRAM_tMRD="8.3";		# ns   
export DRAM_tRAS="26.0";		# ns   
export DRAM_tRAP="5.6";		# ns   
export DRAM_tRCD="6.0";		# ns   
export DRAM_tRFC="84.6";		# ns   
export DRAM_tRP="6.3";		# ns   
export DRAM_tRRD="2.8";		# ns
export DRAM_tWR="4.5";		# ns
export DRAM_tDLL_OFF="2.8";	# ns
# Manual DRAM Power Model
export DRAM_IDLE_base_load="0.020834 * (1.0 / (tCK * 0.001)  )  +	4.05"; # mA
export DRAM_IDLE_base_load_dll_off=" 0.010347 *  (1.0 / (tCK * 0.001)  )  + 2.64"; # mA
export DRAM_IACT_bass_add_per_bank="0.3"; # mA
export DRAM_QACT="2305.1"; # pC
export DRAM_QPRE="1226.3"; #pC				
export DRAM_IRDWR_base_adder="4.8"; #mA				
export DRAM_QREAD="729.8"; #pC				
export DRAM_QWRITE="655.4"; #pC				
export DRAM_IPWRDOWN="2.4"; #mA				
export DRAM_ISELFREFRESH="5.1"; #mA				
export DRAM_QREFRESH="28251.4"; #pC				
export DRAM_FILE_OUT="2048Mbit_128IOs_8Banks_DDR_45nm_meas.txt"

# Wait for DRAM to initialize, before injecting transactions (Cycles)
#  0, 1, 2, ...
export DRAM_INIT_TIME=24000
