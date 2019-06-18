### SIMULATION WITH TRAFFIC GENERATORS

import optparse
import sys
import subprocess

import m5
from m5.objects import *
from m5.util import addToPath
from m5.internal.stats import periodicStatDump

#*****************************
import ethz_utils
from ethz_utils import *
#*****************************

addToPath('../common')
import MemConfig

#*****************************
import ethz_configs
from ethz_configs import *
import _gem5_params
from _gem5_params import *
#*****************************

# this script is helpful to sweep the efficiency of a specific memory
# controller configuration, by varying the number of banks accessed,
# and the sequential stride size (how many bytes per activate), and
# observe what bus utilisation (bandwidth) is achieved

parser = optparse.OptionParser()

# Use a single-channel DDR3-1600 x64 by default
parser.add_option("--mem-type", type="choice", default="ddr3_1600_x64",
                  choices=MemConfig.mem_names(),
                  help = "type of memory to use")

parser.add_option("--ranks", "-r", type="int", default=1,
                  help = "Number of ranks to iterate across")

parser.add_option("--rd_perc", type="int", default=100,
                  help = "Percentage of read commands")

#parser.add_option("--mode", type="choice", default="DRAM",
                  #choices=["DRAM", "DRAM_ROTATE"],
                  #help = "DRAM: Random traffic; \
                          #DRAM_ROTATE: Traffic rotating across banks and ranks")

parser.add_option("--addr_map", type="int", default=1,
                  help = "0: RoCoRaBaCh; 1: RoRaBaCoCh/RoRaBaChCo")

(options, args) = parser.parse_args()

options.mode = GEM5_TRAFFIC_MODE

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# at the moment we stay with the default open-adaptive page policy,
# and address mapping

# start with the system itself, using a multi-layer 1.5 GHz
# crossbar, delivering 64 bytes / 5 cycles (one header cycle)
# which amounts to 19.2 GByte/s per layer and thus per port
system = System()
system.clk_domain = SrcClockDomain(clock = '100GHz',
                                   voltage_domain =
                                   VoltageDomain(voltage = '1V'))

# we are fine with 256 MB memory for now
system.membus = NoncoherentXBar( width=AXI_DATA_W/8 ) #width=SERIALLINKS_NUM_LANES_PER_LINK * SERIALLINKS_NUM_LINKS );
system.membus.clk_domain = SrcClockDomain(clock='100GHz', voltage_domain = VoltageDomain(voltage = '1V'))

mem_range = AddrRange(TOTAL_MEM_SIZE_MB)
system.mem_ranges = [mem_range]

# force a single channel to match the assumptions in the DRAM traffic
# generator

# MEMORY CONFIGURATION
ethz_config_memory(system, options);

# stay in each state for 0.25 ms, long enough to warm things up, and
# short enough to avoid hitting a refresh
period = GEM5_TRAFFIC_ACTIVE_PERIOD_ps #GEM5_TOTAL_SIMULATION_PERIOD  # ERFAN

# this is where we go off piste, and print the traffic generator
# configuration that we will later use, crazy but it works
cfg_file_name = OUTDIR + "/traffic.cfg"
cfg_file = open(cfg_file_name, 'w')

# stay in each state as long as the dump/reset period, use the entire
# range, issue transactions of the right DRAM burst size, and match
# the DRAM maximum bandwidth to ensure that it is saturated

# get the number of banks
#nbr_banks = system.mem_ctrls[0].banks_per_rank.value

# determine the burst length in bytes
burst_size = SMC_BURST_SIZE_B # because burst size is in bits
	
# if ( burst_size != 256 ) :
# 	print "Error: burst size must be 256 Bytes"
# 	exit

page_size = SMC_BURST_SIZE_B

# match the maximum bandwidth of the memory, the parameter is in ns
# and we need it in ticks
itt = GEM5_TRAFFIC_ITT_ps;#system.mem_ctrls[0].tBURST.value * 1000000000000

# assume we start at 0
max_addr = mem_range.end

# use min of the page size and 512 bytes as that should be more than
# enough
max_stride = min(512, page_size)

# now we create the state by iterating over the stride size from burst
# size to the max stride, and from using only a single bank up to the
# number of banks available
#nxt_state = 0
#for bank in range(1, nbr_banks + 1):
    #for stride_size in range(burst_size, max_stride + 1, burst_size):
        #cfg_file.write("STATE %d %d %s %d 0 %d %d "
                       #"%d %d %d %d %d %d %d %d %d\n" %
                       #(nxt_state, period, options.mode, options.rd_perc,
                        #max_addr, burst_size, itt, itt, 0, stride_size,
                        #page_size, nbr_banks, bank, options.addr_map,
                        #options.ranks))
        #nxt_state = nxt_state + 1
        
cfg_file.write("STATE %d %d %s %d 0 %d %d "
	"%d %d %d\n" %
	(0, period, options.mode, options.rd_perc,
	max_addr, burst_size, itt, itt, 0 ))
	

#cfg_file.write("STATE %d %d %s %d 0 %d %d "
	#"%d %d %d\n" %
	#(1, GEM5_TRAFFIC_IDLE_PERIOD_ps, options.mode, options.rd_perc,
	#max_addr, burst_size, GEM5_TRAFFIC_IDLE_PERIOD_ps-1, GEM5_TRAFFIC_IDLE_PERIOD_ps-1, 0 ))

cfg_file.write("STATE %d %d %s\n" %
	(1, GEM5_TRAFFIC_IDLE_PERIOD_ps, "IDLE"))



	#, burst_size,
	#page_size, nbr_banks, nbr_banks, options.addr_map,
	#options.ranks))
        

cfg_file.write("INIT 0\n")

# go through the states one by one
#for state in range(1, nxt_state):
    #cfg_file.write("TRANSITION %d %d 1\n" % (state - 1, state))

cfg_file.write("TRANSITION %d %d 1\n" % (0, 1))
cfg_file.write("TRANSITION %d %d 1\n" % (1, 0))

cfg_file.close()

np = GEM5_NUMCPU
# create a traffic generator, and point it to the file we just created
system.tgen = [ TrafficGen(config_file = cfg_file_name) for i in xrange(np)]
for i in xrange(np):
	system.tgen[i].port = system.membus.slave
	system.tgen[i].NUM_TRANSACTIONS = GEM5_TRAFFIC_NUM_TRANS
	system.tgen[i].MAX_INFLIGHT_TRANS = MAX_INFLIGHT_TRANS
	if ( GEM5_TRAFFIC_PRINT_INJECTED == "TRUE" ):
		system.tgen[i].PRINT_INJECTED_PACKETS = True;
	if ( GEM5_TRAFFIC_PRINT_RECEIVED == "TRUE" ):
		system.tgen[i].PRINT_RECEIVED_PACKETS = True;
	if ( GEM5_RECORD_INJECTED_TRACES == "TRUE" ):
		system.tgen[i].RECORD_INJECTED_TRACES = True;
	system.tgen[i].RECORD_FILE_NAME = GEM5_RECORD_FILE_NAME;

 # connect the system port even if it is not used in this example
system.system_port = system.membus.slave

# every period, dump and reset all stats
if ( GEM5_PERIODIC_STATS_DUMP == "TRUE" ):
	periodicStatDump(GEM5_PERIODIC_STATS_DUMP_PERIOD)

# run Forrest, run!
root = Root(full_system = False, system = system)
root.system.mem_mode = 'timing'

print colors.WARNING + "Traffic: Read_Ratio:" + str(options.rd_perc) + "%\tITT:" + str(itt) + "\tMode:" + options.mode + colors.ENDC;
print colors.WARNING + "Setting the clock frequency of the traffic generators intentionally to 100GHz (membus should not become the bottleneck)" + colors.ENDC;

ethz_perform_sanity_checks(system)

m5.instantiate()
if ( GEM5_TOTAL_SIMULATION_PERIOD > 0 ):
	m5.simulate(GEM5_TOTAL_SIMULATION_PERIOD)
else:
	m5.simulate()

ethz_print_msg("Gem5 finished!")


#print "DRAM sweep with burst: %d, banks: %d, max stride: %d" % \
    #(burst_size, nbr_banks, max_stride)
