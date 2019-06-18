#!/bin/bash
set -e

###########################################################################
###########################################################################
# Paths
#*************************************************************
export SMC_BASE_DIR=$(cat UTILS/variables/SMC_BASE_DIR.txt)
export SMC_WORK_DIR=$(cat UTILS/variables/SMC_WORK_DIR.txt)
#*************************************************************

# Global directories
export SMC_UTILS_DIR=$SMC_BASE_DIR/UTILS
export SMC_VARS_DIR=$SMC_UTILS_DIR/variables/
export SCENARIOS_DIR=$SMC_WORK_DIR/scenarios/

# gem5 directories
export GEM5_BASE_DIR=$SMC_BASE_DIR/GEM5/gem5/
export GEM5_BUILD_DIR=$SMC_WORK_DIR/gem5-build/
export GEM5_UTILS_DIR=$SMC_BASE_DIR/GEM5/utils/

# Software directories
export SW_DIR=$SMC_BASE_DIR/SW/
export PIM_SW_DIR=$SW_DIR/PIM/
export CLUSTER_SW_DIR=$SW_DIR/CLUSTER/
export HOST_SW_DIR=$SW_DIR/HOST/

# HDL directories
export LIB="work"
export VSIM_BASE_DIR="$SMC_BASE_DIR/HDL/VSIM/"
export SYNTH_BASE_DIR="$SMC_BASE_DIR/HDL/SYNTH"         # Do not put / in the righthand side
export SIGNOFF_BASE_DIR="$SMC_BASE_DIR/HDL/SIGNOFF"     # Do not put / in the righthand side
export RTL_PATH="$SMC_BASE_DIR/HDL/RTL"
export AXI_NODE_PATH="$RTL_PATH/AXI_NODE"
export DRAM_CH_PATH="$RTL_PATH/DRAM_CH"
export GLOBALS_PATH="$RTL_PATH/GLOBALS"
export TOP_PATH="$RTL_PATH/TOP"
export TB_PATH="$VSIM_BASE_DIR/TB"

# Scrambler
export SCRAMBLER_UTILS_DIR=$GLOBALS_PATH/scrambler/
export SCRAMBLER_WORK_DIR=$SMC_WORK_DIR/scrambler/

# Misc. directories
export CACTI=cacti65
export TRACE_UTILS_DIR=$SMC_BASE_DIR/UTILS/trace/

export __SCENARIO_NAME="Default scenario"
export __SCENARIO=default
export __CASE=default

export GNUPLOT_DEFAULT_TERMINAL=$(cat UTILS/variables/GNUPLOT_DEFAULT_TERMINAL.txt)

###########################################################################
source $SMC_BASE_DIR/UTILS/common_utils.sh
###########################################################################
# Notice: Array Parameters Indices: ( 0 1 2 3 4 ... )
# [Simulation Only]
export NUM_PARALLEL_SIMULATIONS=4		# Maximum number of jobs to simulate in parallel ( in case of using -m option)
export UNIFORM_CPU_LOAD="TRUE"			# TRUE: maintain a uniform load on the CPUs, FALSE: Run N jobs, wait to finish, run N others

# [Simulation Only]
# This parameter allows user to reduce the total memory without affecting performance and timing results (Of course data may return incorrectly from the memory).
#  For example a MEMORY_SIZE_REDUCTION_FACTOR=4 reduces total allocated memory by a factor of 4, without reflecting it in the simulation
export MEMORY_SIZE_REDUCTION_FACTOR=1		# 1, 2, 4, 8, ...

# [Post Simulation]
export POST_SYNTHESIS_SIMULATION="FALSE"		# "TRUE", "FALSE"
export POST_SYNTHESIS_NETLIST=""			# The netlist which contains the synthesized design
export AXI_NODE_MODEL="axi_node"			# Default: behavioral model - {axi_node, axi_node_ps, axi_node_golden}
export AXI_DRAM_IF_MODEL="axi_dram_if"		# Default: behavioral model
export AXI_CH_CTRL_MODEL="axi_ch_ctrl"		# Default: behavioral model
export TECHNOLOGY_LIBRARY_FUNCTIONAL_PATH="/home/erfan/projects/LIBS/ModelSim/CMOS028_Func"	# Functional Post Simulation
export TECHNOLOGY_LIBRARY_TIMING_PATH="/home/erfan/projects/LIBS/ModelSim/CMOS028"		# Timing Post Simulation
export DUMP_VCD=FALSE
export DUMP_VCD_NAME=NONE       # Name of the subdesign to dump vcd for (you have to look inside VSIM/TB/testbench_smc.sv)
export DUMP_VCD_LOCATION=NONE

# [Trace-based Simulation (Open-loop)]
export TRACE_FOLDER_LOCATION="$SMC_WORK_DIR/traces"	# Base folder for the traces
export TRACE_NAME="blackscholes"			# Name of the trace to apply
export TRACE_TIME_UNIT_ps=1			# >0: Trace time unit (in picoseconds) - 0: ignore timestamp of the traces and apply them simultaneously
export TRACE_MAX_COUNT=1000			# Maximum number of traces to inject
export TRACE_FULL_ROW_ACCESS="FALSE"		# "TRUE": We assume that the burst size of the trace is equal to the row buffer (to maximize bandwidth), "FALSE": Normal operation
###########################################################################
###########################################################################
# [Statistical Memory Model (SMM)]
export SMM_SHIFT_REGISTER_LENGTH=8		# Length of the shift register (elements)
export SMM_MAT_SCALE_FACTOR=1000		# Scale MAT to bring it to range of [0,1] (We saturate MATs higher than this value)
export SMM_CLK_SCALE_FACTOR=5.0			# Scale clock to squeeze transactions (better learning of ANN)
export SMM_INACTIVE_VALUE=-1.0			# Inactive value (when there is no address in the shift register)
###########################################################################
###########################################################################
# [Architectural Parameters]

# AXI Node Clock Period. Can be different from tCK (DRAM Clock Period)
export CLK_PERIOD_SYS=1.0	# (ns)

# AXI Bus Parameters
export AXI_DATA_W=128;		#  16, 32, 64, 128, ...	# Data width of the AXI interconnect	
export AXI_BURST_LENGTH=16;	#  1, 2, 4, 8, ...	# Maximum burst size of the AXI Bus (DRAM Burst length is set automatically based on this value)
export N_TARG_PORT=8;		#  1, 2, 3, 4, 5, ...	# Number of Target Ports: (MASTER)	
export N_INIT_PORT=4;		#  1, 2, 3, 4, 5, ...	# Number of Initiator Ports: (SLAVE)	
export N_REGION=1;		#  1, 2, ...		# Each INIT port can be mapped to several regions	
export N_MEM_DIES=1;		#  1, 2, 4, ...		# Number of stacked memory dies
export BURST_MULTIPLEXING="COARSE"	# "FINE": Flit-by-flit, "COARSE": Burst by burst (This parameter is specific to axi_node.sv)

# Low Priority Ports (LPP) are the ports which the PIM devices are connected to
# In an arbitration between a High Priority Port (HPP) and an LPP, always the HPP wins
export ENABLE_LOW_PRIORITY_PORTS="FALSE";	# "TRUE", "FALSE"

# Throttling at the master ports to avoid transactions receive exactly at the same time and get stalled
# this jitter will be applied to all TARGET ports (and will be considered in calculation of MAT)
export THROTTLING_JITTER=0;	#  0, 1, ...

# Address mapping scheme utilized in the single cube:
# CH=Channel, LB=LayerBank, RC=RowColumn, OF=Offset [OF must always remain in the LSB position]
#  MSB     LSB 
P=(CH LB RC OF); export_array ADDRESS_MAPPING P	# Remapping original address from (CH LB RC OF) to this new pattern
P=(CH LB RC OF); export_array DEFAULT_MAPPING P	# DO NOT MODIFY this parameter (This is hard coded in HDL)
P=( 1  1  1  1); export_array ADDRESS_MASKING P	# Mask DEFAULT_MAPPING, e.g. MASK = (0 1 0 1), masked fields= CH, RC (Useful for traffic injection)

# Allocation of Transaction ID (TID) in the traffic generators
#  "ZERO"	: TID = 0 for all transactions
#  "LINEAR"     : TID = TID + 1 until wraps around again to zero (Only for testing)
#  "READY_QUEUE": TID=READY_QUEUE.pop (Stall if no ready TIDs)
export TID_ALLOCATION_MODE="READY_QUEUE"

# Maximum In-flight Transactions (MiT) for one master
#  This parameter also defines the number of bits in AXI_ID_IN = log2(MiT)
export MAX_INFLIGHT_TRANS=44		#  1, 2, 4, 8, ...

#**************************************************************************
#**************************************************************************
# Buffer depth of the FIFOs at the master side of the AXI node
export AXIMASTER_AW_FIFO_DEPTH=4;
export AXIMASTER_W_FIFO_DEPTH=16;
export AXIMASTER_AR_FIFO_DEPTH=4;
export AXIMASTER_R_FIFO_DEPTH=4;
export AXIMASTER_B_FIFO_DEPTH=4;
# Buffer depth of the FIFOs at the slave side of the AXI node
export AXISLAVE_AW_FIFO_DEPTH=4;
export AXISLAVE_W_FIFO_DEPTH=4;
export AXISLAVE_AR_FIFO_DEPTH=16;
export AXISLAVE_B_FIFO_DEPTH=16;
export AXISLAVE_R_FIFO_DEPTH=16;
# Buffer depths of the channel controller
export CHCTRL_W_FIFO_DEPTH=32;
export CHCTRL_R_FIFO_DEPTH=4;
# Bank-level Parallelism
export CHCTRL_BANK_FIFO_DEPTH=8		# Deepak
export CHCTRL_ID_FIFO_DEPTH=64			# Deepak
# Gem5 only: (These parameters do not directly correlate with number of flip-flops in hardware)
#  (Because gem5 does not model flits accurately
export GEM5_VAULT_RBUFFER_SIZE=64
export GEM5_VAULT_WBUFFER_SIZE=64
#**************************************************************************
#**************************************************************************
# [Synthetic Traffic Generation]

# AXI Traffic Generators:
#  "GENERIC"			Capable of automatic testing and performance measurement
#  "IGOR"			Initial traffic generator by Igor Loi
export AXI_TRAFFIC_GEN="GENERIC"

# The working set size (WSS) for the UNIFORM_LOCAL traffic address mode
#  1, 2, 4, 8, ...
P=( 1024 1024 1024 1024 1024 1024 1024 1024 );	export_array AXI_TRAFFIC_WORKING_SET_SIZE P

# Generic AXI Traffic Generator Modes:
#  "MANUAL"			See TGEN_AXI_GENERIC.sv				FOR DEBUGGING
#  "FILL_W"			W(0..N) ...					FOR DEBUGGING
#  "FILL_R"			R(0..N) ...					FOR DEBUGGING
#  "FILL_W_FILL_R"		W(0..N) R(0..N) ...				FOR DEBUGGING
#  "RANDOM_WR"			W(R1) R(R1) W(R2) R(R2) ...			FOR DEBUGGING
#  "RANDOM_W_RANDOM_R"		W(R1) .. R(R2) R(R3) .. W(R4) W(R5) W(R6) ..	PERFORMANCE ANALYSIS
#  "EXTERNAL"			External Traffic Generator			PERFORMANCE ANALYSIS
P=( "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" "RANDOM_W_RANDOM_R" )
export_array AXI_TRAFFIC_MODE P

# Traffic Address Mode of the AXI traffic generator (Only applicable to RANDOM_WR and RANDOM_W_RANDOM_R modes, FIXED_ZERO is applicable to all modes)
#  "FIXED_ZERO"			# Address = 0
#  "UNIFORM_LOCAL"		# Uniform random access to a local working set (see working_set_size)
#  "UNIFORM_ALL"			# Uniform random access to all locations in the memory
#  "STREAM"			# Streaming memory access inside the working_set_size
#  "PIM_DOUBLE_BUFFER"		# Double buffered PIM Traffic: so we only wait for the responses to come back, then we issue another transaction 
P=( "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" "UNIFORM_ALL" )
export_array AXI_TRAFFIC_ADDR_MODE P

# Number of transactions to inject by each master
P=( 1000 1000 1000 1000 1000 1000 1000 1000 );	export_array NUM_TRANSACTIONS P; #  >= 0

# Delay and Jitter between consecutive transactions (cycles)
P=( 0 0 0 0 0 0 0 0 ); 		export_array TRAFFIC_IAT_DELAY P;	# Delay  >= 0
P=( 0 0 0 0 0 0 0 0 );		export_array TRAFFIC_IAT_JITTER P	# Jitter >= 0

# Delay and jitter before injecting the first transaction (cycles)
P=( 0 0 0 0 0 0 0 0 );		export_array TRAFFIC_INITIAL_DELAY P;	# Delay  >= 0
P=( 0 0 0 0 0 0 0 0 );		export_array TRAFFIC_INITIAL_JITTER P	# Jitter >= 0

# Determine which masters should inject traffic
#  1: MASTER[i]: Active
#  0: MASTER[i]: Sleep
P=( 1 1 1 1 1 1 1 1 );		export_array TRAFFIC_MASK P

# Which masters are important to end the simulation
#  1: MASTER[i]: simulation will wait for this master to inject all transactions, then will finish
#  0: MASTER[i]: simulation will not wait for this master, and will finish whenever others have finished
P=( 1 1 1 1 1 1 1 1 );		export_array STOP_SIM_MASK P

# Duration of simulation in (cycles)
#  "100": simulate 100 cycles (after dram has been initialized correctly)
#  "all":   simulate until STOP_SIM flag is received
export CYCLES_TO_SIMULATE="all"

# Number of words in each transaction:
P=( 8 8 8 8 8 8 8 8 );		export_array TRAFFIC_NUM_WORDS P;	#  1, 2, 4, 8

# Randomize number of words in [1, TRAFFIC_NUM_WORDS]:
P=( "TRUE" "TRUE" "TRUE" "TRUE" "TRUE" "TRUE" "TRUE" "TRUE" );	export_array TRAFFIC_RANDOM_NUM_WORDS P;	#  "TRUE", "FALSE"

# Number of consecutive write or read commands in a row (RANDOM_W_RANDOM_R mode): NUM_CONSECUTIVE + Random( VAR_CONSECUTIVE ) 
#  1, 2, 3, ...
P=( 1 1 1 1 1 1 1 1 );		export_array NUM_CONSECUTIVE_WRITES P;	# Writes
P=( 1 1 1 1 1 1 1 1 );		export_array NUM_CONSECUTIVE_READS P;	# Reads
# Variation in the number of write or read commands in a row (RANDOM_W_RANDOM_R mode)
P=( 0 0 0 0 0 0 0 0 );		export_array VAR_CONSECUTIVE_WRITES P;	# Writes
P=( 0 0 0 0 0 0 0 0 );		export_array VAR_CONSECUTIVE_READS P;	# Reads
# Delay after a set of consecutive packets
P=( 0 0 0 0 0 0 0 0 );		export_array DELAY_AFTER_CONSECUTIVE_WRITES P;	# Writes
P=( 0 0 0 0 0 0 0 0 );		export_array DELAY_AFTER_CONSECUTIVE_READS P;	# Reads

#**************************************************************************
#**************************************************************************
# [VSIM Reports and Statistics]

# Halt simulation if incorrect data is returned (TRUE, FALSE)
export HALT_ON_FUNCTIONAL_ERROR=TRUE

# Send sender information via wdata to verify responses:
export VERIFY_READ_WRITE="TRUE";			#  "TRUE", "FALSE"

# Optimization mode of the VSIM simulation
export VSIM_OPT_MODE="-novopt"				#  "-novopt", "-vopt"

# Print information about PASSED transactions (failed transactions will always be dumped)
export REPORT_PASSED_TRANSACTIONS="FALSE";	#  "TRUE", "FALSE"

# Print information about RACED transactions (failed transactions will always be dumped)
export REPORT_RACED_TRANSACTIONS="FALSE";	#  "TRUE", "FALSE"

# Print information about UNINITIALIZED transactions (Reading from uninitialized location of memory)
export REPORT_UNINIT_TRANSACTIONS="FALSE";	#  "TRUE", "FALSE"

# Print address remapping in each packet
export REPORT_ADDRESS_REMAP="FALSE";		#  "TRUE", "FALSE"

# Print simulation duration (ms)
export REPORT_SIMULATION_TIME="FALSE";		#  "TRUE", "FALSE"

# Dump memory access time in results/MAT/<ID>W.txt and results/MAT/<ID>R.txt
# Dump memory access time distribution in results/MAT/<ID>W_dist.txt and results/MAT/<ID>R_dist.txt
export DUMP_MAT="FALSE"				#  "TRUE", "FALSE"
export DUMP_MAT_DISTRIBUTION="FALSE";		#  "TRUE", "FALSE" 
export DUMP_MAT_DETAILS="FALSE"			#  "TRUE", "FALSE" (Detailed report for memory access time [Only for Debugging and for Statistical Calibration])

# Dump total delivered bandwidth periodically (GBps)
export DUMP_BANDWIDTH_PERIODICALLY="FALSE";	#  "TRUE", "FALSE" 
export DUMP_BANDWIDTH_PERIOD=100;		#  Period in cycles

# Dump the number of in-flight transactions in the system over time
export DUMP_INFLIGHT_TRANS="FALSE";		#  "TRUE", "FALSE" 

# Dump statistics for each individual packet (e.g. MAT breakdown)
export DUMP_PACKET_STATS="FALSE";		#  "TRUE", "FALSE"

# Dump the status of all system buffers (FIFOs), and the number of elements in them
export DUMP_BUFFER_STATUS="FALSE";		#  "TRUE", "FALSE"
export DUMP_BUFFER_DISTRIBUTION="FALSE";	#  "TRUE", "FALSE"

# Dump the traffic injected by masters into text files (notice: this happens after address remapping)
export DUMP_TRAFFIC="FALSE";			#  "TRUE", "FALSE"

# Dump power consumption values of the DRAM into (DRAM_FILE_OUT) file
export DUMP_DRAM_POWER="FALSE";			#  "TRUE", "FALSE"

# Dump DRAM commands over time (useful for plotting)
export DUMP_DRAM_COMMANDS="FALSE";		#  "TRUE", "FALSE"

# Display debug messages for the Tukl 3D DRAM:
export DEBUG_TUKL_3DRAM="FALSE"			#  "TRUE", "FALSE"

# Display debug messages for axi_dram_if.sv
export DEBUG_AXI_DRAM_IF="FALSE"			#  "TRUE", "FALSE"

# Display debug messages for the Channel Controller
export DEBUG_CH_CTRL="FALSE"			#  "TRUE", "FALSE"

# Display debug messages for the ch_ctrl_phy.sv
export DEBUG_CH_CTRL_PHY="FALSE"

# Dump the whole contents of TUKL 3D DRAM in MEM/ch?_bank?.txt
#  starting from bank address (START_ADDR) up to (END_ADDR)
export DUMP_TUKL_3DRAM="FALSE";			#  "TRUE", "FALSE"
export DUMP_TUKL_3DRAM_START_ADDR="0";		#  Dump start address
export DUMP_TUKL_3DRAM_END_ADDR="10000"		#  Dump end address
export DUMP_TUKL_3DRAM_AT_TIME_ns="1000"		#  Dump time in (ns)

# Report some minor statistics periodically (progressive report), inside modules
# every PERIODIC_REPORT_PERIOD cycles, some parameters will be reported
#  "FALSE", "TRUE"
export PERIODIC_REPORT_tgen_axi_generic="TRUE"
export PERIODIC_REPORT_3d_dram="FALSE"
export PERIODIC_REPORT_axi_dram_if="FALSE"
export PERIODIC_REPORT_ddr_channels="FALSE"
export PERIODIC_REPORT_addr_fifo="FALSE"
export PERIODIC_REPORT_rw_fsm="FALSE"
export PERIODIC_REPORT_rdata_fifo="FALSE"
export PERIODIC_REPORT_chctrl_phy="FALSE"
export PERIODIC_REPORT_PERIOD=10000;		#  1, 2, ... # Period of the periodic report in cycles

# Measure statistics: "TRUE", "FALSE"
export MEASURE_LINK_STATISTICS="TRUE";		# link statistics such as utilization, bandwidth, stall time, ...
export MEASURE_DDR_STATISTICS="FALSE";		# link statistics of the DDR channels
export MEASURE_CHCTRL_STATISTICS="FALSE"	# Measure channel controller statistics (detailed statistics of Banks, FSMs, DRAM itself)
export MEASURE_AVERAGE_IAT="FALSE"		# Measure average interarrival time of scheduled traffic (suitable for computation/communication ratio)

# Report convered address range: MIN_ACCESSED_ADDRESS and MAX_ACCESSED_ADDRESS 
export REPORT_COVERED_ADDRESS_RANGE="FALSE";	#  "TRUE", "FALSE"

#**************************************************************************
#**************************************************************************
# [Memory Models]

# Memory model utilized in the SMC:
#  "SRAM": Single Cycle Ideal SRAM
#  "DRAM": DRAM Controller + 3DDRAM
export MEMORY_MODEL="DRAM"

# This parameter is normally zero, but if we resume a checkpoint in the closed-loop simulation
#  between gem5 and modelsim, this parameter should define the cycle to resume in gem5 
# (This is automatically done in the smc.sh script)
export VSIM_INIT_CYCLE=0

#---------------------------------------------------------------------------------------
# Models of the 3D DRAM:
#   you can use these dram models directly in the following way:
#   load_memory_model hmc_2011.sh
#---------------------------------------------------------------------------------------

# Vault Controller
export GEM5_VAULTCTRL_FRONTEND_LATENCY_cy=4		# Static Frontend latency of the vault controllers (gem5)
export GEM5_VAULTCTRL_BACKEND_LATENCY_cy=4		# Static Backend latency of the vault controllers (gem5)

# DRAM Parameters
export DRAM_BANK_PARALLELISM=FALSE			# TRUE, FALSE (Parameter is only available in RTL, gem5 by default supports it)	(Bank_Level_Parallelism)
export DRAM_SETUPHOLD_CHECK=TRUE
export DRAM_SCHEDULING_POLICY_GEM5="fcfs"	# "fcfs", "frfcfs (Only for gem5's DRAMCtrl class)
export DRAM_PAGE_POLICY="CLOSED";		# "OPEN", "CLOSED"
export DRAM_BUS_WIDTH="128";			# bit  
export DRAM_BANKS_PER_DIE="2";			# banks in one channel of one memory die
export DRAM_column_size="6";			# bit  
export DRAM_row_size="16";			# bit	# 12, 13, 14, 15, ...
export DRAM_tCL="8.4";				# ns	# CAS Latency
export DRAM_tCK="2.8";				# ns	# DRAM Clk Period
export DRAM_tMRD="8.3";				# ns	# Mode Register
export DRAM_tRTP="7.5";				# ns	# Read to Precharge Latency (Only in gem5 model)
export DRAM_tRAS="26.0";			# ns	# Row Access Strobe   
export DRAM_tRAP="5.6";				# ns	# Activate to Read with Auto Precharge
export DRAM_tRCD="6.0";				# ns	# Activate to Read without Autoprecharge
export DRAM_tRFC="84.6";			# ns	# Refresh Cycle Time
export DRAM_tRP="6.3";				# ns	# RAS Precharge (Latency of the precharge command)
export DRAM_tRRD="2.8";				# ns	# Row to Row Activation Delay (Power Related)
export DRAM_tWR="4.5";				# ns	# Write Recovery Time
export DRAM_tDLL_OFF="2.8";			# ns	# Delay DLL OFF
export DRAM_tINIT0="20";			# ns	# Apply Voltage RAMP - MAX
export DRAM_tINIT1="10";			# ns	# Activate CLK (after tINIT0) for 100ns while cke is still Low  - MIN
export DRAM_tINIT3="40";			# ns	# Wait tINIT3 after CKE before going in PRE_ALL
export DRAM_tREF_PERIOD="64000000";		# ns	# DRAM Refresh period (ns)

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

# Initialize all dram locations for debugging
# The test generator will use the INITIALIZE_DRAM_PATTERN to identify whether this location has been used or not
export INITIALIZE_DRAM=$VERIFY_READ_WRITE;	#  "TRUE", "FALSE"
export INITIALIZE_DRAM_PATTERN="128'hFADEXXXXFADEXXXXFADEXXXXFADEXXXX"; 	# Pattern to initialize the dram with

# Dump access to dram banks over time
export DUMP_DRAM_HEAT_MAP=FALSE

###########################################################################
###########################################################################
# # Parameters of models inside gem5
export GEM5_VERBOSITY="--verbose"			# {--verbose, --quiet}
export GEM5_BARE_METAL_HOST="FALSE"			# Bare metal simulation or running with an operating system
export GEM5_PLATFORM=ARM
export GEM5_SIM_MODE=opt			# debug -> opt -> fast
export GEM5_SIM_SCRIPT=./configs/example/ethz_fs.py
export GEM5_DEBUGFLAGS=""			# "AddrRanges,XBar,Fetch,Decode", "XBar", "DRAM" ... See src/mem/SConscript for available debug flags in the memory system
export GEM5_DEBUGFILE=""			# Redirect the debug output of gem5 to a file
export GEM5_CPUTYPE=timing 			# 'arm_detailed', 'AtomicSimpleCPU', 'TimingSimpleCPU', 'DerivO3CPU', 'MinorCPU', 'timing', 'detailed', 'atomic', 'minor'
export GEM5_NUMCPU=1				# 1 2 3 4 ...
export GEM5_ADDR_MAPPING="RoCoRaBaCh"		# Address mapping in gem5's HMC model
export GEM5_MEMTYPE=HMCVault		# {DDR3_1600_x64, HMCVault, ethz_ModelsimIF, SimpleMemory, dramsim2 ...}
export HOST_CLOCK_FREQUENCY_GHz=1	# Clock frequency of the host (Full ARM System). Notice: clk of the TGENs is different from this value (See: ethz_tgen.py)
export SERIALLINKS_CLOCK_FREQUENCY_GHz=10	# The clock frequency of the serial links
export SERIALLINKS_NUM_LINKS=4				# Number of serial links
export SERIALLINKS_NUM_LANES_PER_LINK=16	# Number of R+W lanes in each serial links
export HAVE_LISTENERS="TRUE"			# Listening to telnet or vnc inside gem5
export GEM5_SMCCONTROLLER_BUFFER_SIZE_REQ=256	# The buffer size of the SMCController inside gem5 (Request)	(Number of packets, not flits)
export GEM5_SMCCONTROLLER_BUFFER_SIZE_RSP=256	# The buffer size of the SMCController inside gem5 (Response)	(Number of packets, not flits)

################################################################################
# # Latency parameters
export GEM5_LINK_BUFFER_SIZE_REQ=16		# The buffer size after the serial links inside gem5 (Request)	 		(Number of packets, not flits)
export GEM5_LINK_BUFFER_SIZE_RSP=16		# The buffer size after the serial links inside gem5 (Response)			(Number of packets, not flits)
export GEM5_SERDES_LATENCY_ns=1.6		# Latency of the SER at the link source, plus DES at the link destination
export GEM5_LINK_LATENCY_ns=3.2			# Latency of the serial link on the PCB
export GEM5_SMCCONTROLLER_LATENCY_cy=8	# Pipeline latency of the SMC Controller (Cycles) [Clock = HOST_CLOCK_FREQUENCY_GHz]

################################################################################

# # Host Cache Parameters
export HAVE_L1_CACHES="TRUE"			# CPUs have L1 caches {TRUE, FALSE}
export HAVE_L2_CACHES="TRUE"			# CPUs have L2 caches {TRUE, FALSE}
export L1I_CACHE_SIZE=NONE				# L1 Instruction Cache Size
export L1D_CACHE_SIZE=NONE				# L1 Data Cache Size
export L2_CACHE_SIZE=NONE				# L2 Cache Size
export HOST_BURST_SIZE_B=256			# CACHE_LINE_SIZE_B of the host processors (can be different from the burst size of the SMC)

################################################################################
# # Closed loop simulation between gem5 and modelsim
export GEM5_CLOSEDLOOP_MODELSIM="FALSE"
export GEM5_REPORT_PERIOD_ps=500000000		# periodic report in gem5
export GEM5_MAX_BUFFER_SIZE=100000		# maximum buffer size to read from linux pipes (must be large, otherwise overflow happens)
export GEM5_SYNCH_PERIOD_ps=50000		# period of synchronization between gem5 and modelsim
export GEM5_SHORT_SLEEP_ns=1			# short sleep
export GEM5_LONG_SLEEP_ns="2*$GEM5_SYNCH_PERIOD_ps/1000"	# long sleep
################################################################################

################################################################################
# # Synthetic traffic generation inside gem5
export GEM5_TRAFFIC_ITT_ps=25000		# Theoretical interarrival time between packets
export GEM5_TRAFFIC_MODE="RANDOM"		# {RANDOM, ZERO}
export GEM5_TRAFFIC_NUM_TRANS=10000		# Number of transactions injected (put -1 for infinite traffic generation)
export GEM5_TRAFFIC_IDLE_PERIOD_ps=500000		
export GEM5_TRAFFIC_ACTIVE_PERIOD_ps=500000		
export GEM5_TOTAL_SIMULATION_PERIOD=2500000000	# Total simulation duration in synthetic traffic mode, put -1 for infinite simulation time
export GEM5_TRAFFIC_PRINT_INJECTED="FALSE"	# Print injected transactions (synthetic traffic generator)
export GEM5_TRAFFIC_PRINT_RECEIVED="FALSE"	# Print received transactions (synthetic traffic generator)
export GEM5_RECORD_INJECTED_TRACES="FALSE"	# Record injected traces in a file
export GEM5_RECORD_FILE_NAME="traces.txt"
################################################################################

################################################################################
# # Statistics inside gem5
export GEM5_ENABLE_COMM_MONITORS="TRUE"	# Enable communication monitors (measure bandwidth, latency, ...)
export GEM5_PERIODIC_STATS_DUMP="FALSE"		# Dump statistics periodically
export GEM5_PERIODIC_STATS_DUMP_PERIOD=1000000000	# Period of dumping the statistics (ps)
################################################################################

# # Simple Memory Model (Only for test)
export GEM5_SIMPLEMEMORY_LATENCY="1ns"		# We can replace the vaults with SimpleMemory for testing purposes
export GEM5_SIMPLEMEMORY_BW="32GB/s"			# We can replace the vaults with SimpleMemory for testing purposes

# # DRAMSim2 Parameters (Only if GEM5_MEMTYPE=dramsim2)
export DRAMSIM2_ENABLE_DEBUG="FALSE"		# Enable debugging in DRAMSim2 (To enable debugging also modify: UTILS/models/memory/dramsim2_system.sh)
export DRAMSIM2_ENABLE_TIMESTAMP="FALSE"	# Enable reporting time when debugging

# # Software Layer
export M5_PATH=${SMC_WORK_DIR}/gem5-images/aarch-system-2014-10/
export HOST_CROSS_COMPILE=NONE		# {arm-linux-gnueabihf-, aarch64-linux-gnu-} ARM Cross Compiler (for the simulated ARM soc, not to be confused with the x86 host machine)
export PIM_CROSS_COMPILE=NONE		# {arm-linux-gnueabihf-, aarch64-linux-gnu-} ARM Cross Compiler (for the pim processor)
export COMPILED_KERNEL_PATH=NONE
export ARCH="NONE"				# Architecture for building the kernel and the drivers
export GEM5_DISKIMAGE="NONE"
export GEM5_MACHINETYPE="NONE"
export GEM5_DTBFILE="NONE"
export GEM5_KERNEL="NONE"			# Kernel on the main CPUs
export GEM5_CHECKPOINT_RESTORE="FALSE"		# If true, system will be restored from previously recorded checkpoint
export GEM5_CHECKPOINT_LOCATION="NONE"		# Full address of the checkpoint to restore
export GEM5_EXTRAIMAGE="${SMC_WORK_DIR}/gem5-images/extra.img"
export SUDO_COMMAND=sudo			# Command to become superuser

# Automated gem5 simulation: takes a checkpoint automatically and stores it. Next time, tries to resume the checkpoint and run the script
export GEM5_AUTOMATED_SIMULATION=FALSE	
###########################################################################
###########################################################################
# # Parameters of the PIM device inside gem5
export HAVE_PIM_DEVICE="FALSE"          # "TRUE", "FALSE"
export HAVE_PIM_CLUSTER="FALSE"         # "TRUE", "FALSE" have a cluster of PIM processors
export HAVE_PARALLEL_PIM="FALSE"        # "TRUE", "FALSE"
export MOVE_PIM_TO_HOST="FALSE"			# "TRUE": PIM is placed on the host side, "FALSE": PIM is placed on the LoB of the HMC (Host Side Accelerator)
export PIM_ADDRESS_BASE="NONE"			# Base address of the memory mapped region inside the PIM device
export PIM_ADDRESS_SIZE="NONE"			# Size of the memory mapped region inside the PIM device (This is not necessarily memory, and can be IO)
export PIM_CLOCK_FREQUENCY_GHz="NONE"	# Clock frequency of the PIM device
export PIM_VREG_SIZE=0				    # Vector size in bytes to perform vector operations
export PIM_SREG_COUNT=0				    # Number of scalar registers on PIM
export GEM5_PIM_KERNEL="NONE"			# Kernel on the pim CPU
export PIM_DTLB_SIZE=0					# Number of rules in the DTLB
export PIM_DTLB_DUMP_ADDRESS="FALSE"	# Dump the data accesses of the DTLB
export PIM_SPM_ACCESSTIME_ns=NONE		# Access time of the Scratchpad memory on PIM (Bandwidth = 32b x PIM_CLOCK_FREQUENCY Gbps)
export SMON_DUMP_ADDRESS="FALSE"        # Dumpy all accesses in the SMon
export HMON_DUMP_ADDRESS="FALSE"        # Dumpy all accesses in the SMon (GATHER TRACES - TRACE GATHERING IN GEM5)

export PIM_TEXT_OFFSET=NONE
export PIM_RODATA_OFFSET=NONE
export PIM_ARRAY_OFFSET=NONE
export PIM_GOT_OFFSET=NONE
export PIM_DATA_OFFSET=NONE
export PIM_BSS_OFFSET=NONE
export PIM_STACK_OFFSET=NONE

export PIM_FIRMWARE_CHECKS=TRUE         # {TRUE, FALSE}
export DEBUG_PIM_RESIDENT=TRUE			# {TRUE, FALSE}
export DEBUG_PIM_DRIVER=TRUE			# {TRUE, FALSE}
export DEBUG_PIM_API=TRUE				# {TRUE, FALSE}
export DEBUG_PIM_APP=TRUE				# {TRUE, FALSE}

# Computation Kernel to offload to PIM
export OFFLOADED_KERNEL_NAME=""           # SEE: SMC/SW/PIM/kernels
export OFFLOADED_KERNEL_SUBNAME="default" # SEE: SMC/SW/PIM/kernels
export OFFLOAD_THE_KERNEL=TRUE			# TRUE: actually offload the kernel (partial offloading)  FALSE: execute the preloaded kernel
export PIM_OPT_LEVEL=""                 # gcc optimization level for PIM  {"", "-O0", "-O3", ...}
export HOST_OPT_LEVEL=""                # gcc optimization level for Host {"", "-O0", "-O3", ...}

###########################################################################
###########################################################################
###########################################################################
# Energy Consumption Models

export SERIALLINKS_ENERGY_PER_BIT=13.7              # (pj/bits) energy consumed in the serial links
export SERIALLINKS_IDLE_POWER=1980                  # (mW) Estimated based on maximum power and link efficiency [Paul Rosenfeld]
export CACHE_TECHNOLOGY_NODE_um=0.032               # (um) Cache Technology node (CACTI) { from 32nm to 90nm }
export OPERATING_TEMPERATURE=300                    # (K) Operating Temperature
export SMCXBAR_ENERGY_PER_BIT=4e-1                  # (pj/bits) energy consumed in the SMCXbar
export MEMBUS_ENERGY_PER_BIT=3e-2                   # (pj/bits) energy consumed in the Membus
export TOL2BUS_ENERGY_PER_BIT=1.8e-1                # (pj/bits) energy consumed in the Tol2bus
export SMCCTRL_ENERGY_PER_BIT=10                    # (pj/bits) energy consumed in the SMC Controller
export VAULTCTRL_ENERGY_PER_BIT=0.75                # (pj/bits) energy consumed in the Vault Controller [FR-FCFS LPDDR2 Controller]
export CPU_IDLE_POWER_AT_2GHZ=500                   # (mW) idle power in the Cortex A9 processors
export CPU_ACTIVE_POWER_AT_2GHZ=1700                # (mW) active power in the Cortex A9 processors
export CPU_LEAKAGE_POWER=60                         # (mW) leakage power in the Cortex A9 processors
export PIM_PERIPHERAL_POWER=20                      # (mW) power consumed in PIM peripherals (estimate): 8KB of SPM + TLB + PIMBus + ...
export ATOMIC_FADD_ENERGY=1.8                       # (pj) energy of one atomic operation (floating point add) - 28nm SOI 0.8V TYP 25C
export ATOMIC_MIN_ENERGY=0.2                        # (pj) energy of one atomic operation (integer min)
export ATOMIC_INC_ENERGY=0.1                        # (pj) energy of one atomic operation (integer inc)

# CPU Energy Models
export CPU_TLB_ENERGY_PER_ACCESS=0.85               # (pj/acc)  64-entry TLB on the host processors

###########################################################################
# Default parameters for synthesis
###########################################################################
set -e
# Maximum number of cores to run Design Compiler synthesis on
export DC_MAX_CORES=4
# Override the parameters of the top module in the elaboration phase: "ADDR_WIDTH=>32,DATA_WIDTH=>64"
export ELABORATION_PARAMETERS=""
# Specify parameters for compilation, you can also specify "" for no parameters
export COMPILATION_PARAMETERS="-no_autoungroup -no_boundary_optimization"
# Specify the ungroup command
export UNGROUP_PARAMETERS="ungroup -flatten -all -force -start_level 1"

#  Dont forget to export TOP_DESIGN_NAME to your design to be synthesized
#  In order to mask synthesis messages per scenario create this file: SYNTH/masked_messages.txt
###########################################################################

# Kernel to execute on the cluster
export CLUSTER_KERNEL_NAME=""

# Host Side cluster
export MOVE_CLUSTER_TO_HOST="FALSE"

# Number of slave CPUs in the cluster
export CLUSTER_NUMSLAVECPUS=1

export PIMDMA_CLOCK_FREQUENCY_GHz=0

export PIMDMA_NUMPORTS=0

# Gather stats or not
export GATHER_CLUSTER_STATS=FALSE

# Progressive Simulation in gem5
export GEM5_PROGRESSIVE_SIMULATION=FALSE

# 0 means: simulate until the application finishes
export DURATION_OF_SIMULATION_ms=0

###########################################################################
