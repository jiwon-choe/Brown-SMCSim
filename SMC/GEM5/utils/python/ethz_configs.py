#TODO ERFAN
import m5
import m5.objects
from m5.objects import *
import inspect
from textwrap import  TextWrapper
import ethz_utils
from ethz_utils import *
import ethz_pimconf
from ethz_pimconf import *
import sys
import _gem5_params
from _gem5_params import *
import MemConfig
import Simulation

if ( HAVE_PIM_DEVICE == "TRUE" ):
    pim_device_range= AddrRange(PIM_ADDRESS_BASE,size = PIM_ADDRESS_SIZE)

####################################
# Configure the PIM device
def ethz_config_pim( test_sys, options ):
    (TestCPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
    pim_sys = build_pim_system(options, test_mem_mode, TestCPUClass)
    test_sys.pim_sys = pim_sys

    pim_sys.p2s = Bridge(ranges=test_sys.mem_ranges, delay='0.01ns', req_size=32, resp_size=32)
    pim_sys.s2p = Bridge(ranges=pim_device_range, delay='0.01ns', req_size=32, resp_size=32)

    if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
        pim_sys.Smon = CommMonitor()

        if ( SMON_DUMP_ADDRESS == "TRUE" ):
            pim_sys.Smon.dump_addresses=True
            pim_sys.Smon.dump_file="m5out/smon_addr_dump.txt"

        pim_sys.pimbus.master = pim_sys.Smon.slave
        pim_sys.Smon.master = pim_sys.p2s.slave
    else:
        pim_sys.pimbus.master = pim_sys.p2s.slave

    pim_sys.s2p.master = pim_sys.pimbus.slave
    if ( MOVE_PIM_TO_HOST == "FALSE" ):
        pim_sys.p2s.master = test_sys.smcxbar.slave
        test_sys.smcxbar.master = pim_sys.s2p.slave;
    else:
        pim_sys.p2s.master = test_sys.membus.slave
        test_sys.membus.master = pim_sys.s2p.slave;

    ### EXTERNAL PIM DEVICE:
    #ethz_print_msg("NOTICE: This should be used only for external C++ PIM model!");
    #test_sys.pim_if= ethz_PIMIF(ranges=pim_device_range);
    #test_sys.pim_if.master = test_sys.smcxbar.slave
    #test_sys.pim_if.slave = test_sys.smcxbar.master

####################################
def ethz_print_smc_config( system, options ):
    fo = open(OUTDIR+"/system.vaults.params.txt", "w")
    fo.write(TOTAL_MEM_SIZE_MB + "\t\t\t\t" + "TOTAL_MEM_SIZE_MB\n");
    fo.write(DRAM_CHANNEL_SIZE_MB + "\t\t\t\t" + "DRAM_CHANNEL_SIZE_MB\n");
    fo.write(DRAM_LAYER_SIZE_MB + "\t\t\t\t" + "DRAM_LAYER_SIZE_MB\n");
    fo.write(str(SMC_BURST_SIZE_B) + "\t\t\t\t" + "SMC_BURST_SIZE_B\n");
    fo.write(str(HOST_BURST_SIZE_B) + "\t\t\t\t" + "HOST_BURST_SIZE_B (cache_line_size)\n");
    fo.write(str(options.mem_channels) + "\t\t\t\t" + "mem_channels\n");
    fo.write("\nVault Parameters:\n");
    ethz_pretty_print_to_file( system.mem_ctrls[0].__dict__.get('_hr_values'), fo )
    fo.write("\nAddress Interleaving Bits:\n");
    fo.write(str(NBITS_CH) + "\t\t\t\t" + "NBITS_CH	(Channels or vaults)\n");
    fo.write(str(NBITS_LB) + "\t\t\t\t" + "NBITS_LB	(Banks in all layers\n");
    fo.write(str(NBITS_OF) + "\t\t\t\t" + "NBITS_OF	(Offset bits)\n");
    fo.write(str(NBITS_RC) + "\t\t\t\t" + "NBITS_RC	(Row bits + column bits)\n");
    fo.write(str(DRAM_COLUMNS_PER_ROW) + "\t\t\t\t" + "COLUMNS_PER_ROW (Number of columns in each row)\n");
    fo.close()

    # print CPU parameters
    # ethz_pretty_print_to_file
	
####################################
# Config SMC based on given parameters in _gem5_params
def ethz_config_smc_vault( vault ):
		vault.page_policy = 'open_adaptive' if DRAM_PAGE_POLICY == 'OPEN' else 'close_adaptive'
		vault.channels = N_INIT_PORT
		vault.ranks_per_channel = N_MEM_DIES          # Each DRAM layer is one rank
		vault.addr_mapping = GEM5_ADDR_MAPPING
		vault.device_size = DRAM_LAYER_SIZE_MB
		vault.device_bus_width = DRAM_BUS_WIDTH
		vault.burst_length = DRAM_BURST_LENGTH
		vault.device_rowbuffer_size = DRAM_ROW_BUFFER_SIZE
		vault.banks_per_rank = DRAM_BANKS_PER_DIE
		vault.tCK   = DRAM_tCK
		vault.tBURST= DRAM_tBURST
		vault.tRCD  = DRAM_tRCD 
		vault.tCL   = DRAM_tCL
		vault.tRP   = DRAM_tRP  
		vault.tRAS  = DRAM_tRAS 
		vault.tRRD  = DRAM_tRRD 
		vault.tRFC  = DRAM_tRFC 
		vault.tWR   = DRAM_tWR  
		vault.tRTP  = DRAM_tRTP
		vault.tREFI = DRAM_tREFI
		vault.write_buffer_size = GEM5_VAULT_WBUFFER_SIZE
		vault.read_buffer_size =  GEM5_VAULT_RBUFFER_SIZE
		vault.mem_sched_policy=DRAM_SCHEDULING_POLICY_GEM5
		vault.clk_domain = SrcClockDomain(clock = VAULT_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))
		vault.static_frontend_latency = VAULTCTRL_FRONTEND_LATENCY
		vault.static_backend_latency = VAULTCTRL_BACKEND_LATENCY
		#vault.activation_limit=?
		#vault.tCS = '?';
		#vault.tWTR = '?';
		#vault.tRTW = '?';

####################################
# Configure system memory based on the given parameters
# Memory is either gem5 internal or modelsim external
def ethz_config_memory( test_sys, options ):
    test_sys.cache_line_size=HOST_BURST_SIZE_B

    if ( GEM5_MEMTYPE == "ethz_ModelsimIF" or GEM5_MEMTYPE == "ethz_modelsim_if" ):
        test_sys.smcxbar = NoncoherentXBar(width=32, header_cycles=1)	# This should not be a bottleneck in closed-loop simulation with vsim
        test_sys.smcxbar.clk_domain = SrcClockDomain(clock='500GHz', voltage_domain = VoltageDomain(voltage = '1V'))
    else:
        test_sys.smcxbar = NoncoherentXBar(width = AXI_DATA_W/8, header_cycles=1)
        test_sys.smcxbar.clk_domain = SrcClockDomain(clock=SMCXBAR_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))

    # Full System Simulation
    #########################
    if ( HAVE_PIM_CLUSTER == "FALSE" ):
        test_sys.smccontroller_pipeline = Bridge(ranges=test_sys.mem_ranges,
            req_size=GEM5_SMCCONTROLLER_BUFFER_SIZE_REQ, resp_size=GEM5_SMCCONTROLLER_BUFFER_SIZE_RSP, delay=GEM5_SMCCONTROLLER_LATENCY )

        # # Serial Links
        # test_sys.seriallink =[ SerialLink(ranges = test_sys.mem_ranges, 
        #     req_size=(GEM5_LINK_BUFFER_SIZE_REQ),
        #     resp_size=(GEM5_LINK_BUFFER_SIZE_RSP),
        #     serialization_unit=SERIALLINKS_NUM_LANES_PER_LINK,
        #     delay=GEM5_LINK_STATIC_LATENCY) for i in xrange(N_TARG_PORT)]

        # Serial Links
        test_sys.seriallink =[ Bridge(ranges = test_sys.mem_ranges, 
            req_size=(GEM5_LINK_BUFFER_SIZE_REQ),
            resp_size=(GEM5_LINK_BUFFER_SIZE_RSP),
            serialization_enable=True,
            serialization_unit=SERIALLINKS_NUM_LANES_PER_LINK,
            delay=GEM5_LINK_STATIC_LATENCY) for i in xrange(N_TARG_PORT)]

        if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
            test_sys.Lmon =[ CommMonitor() for i in xrange(N_TARG_PORT)]       
            
        # SMC Controller
        test_sys.smccontroller=LoadDistributor( width=SERIALLINKS_NUM_LANES_PER_LINK * SERIALLINKS_NUM_LINKS/8, header_cycles=8 )
        test_sys.smccontroller.clk_domain = SrcClockDomain(clock=SERIALLINKS_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))

        if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
            test_sys.Hmon = CommMonitor()

            if ( HMON_DUMP_ADDRESS == "TRUE" ):
                test_sys.Hmon.dump_addresses=True
                test_sys.Hmon.dump_file="m5out/hmon_addr_dump.txt"

            test_sys.membus.master = test_sys.Hmon.slave
            test_sys.Hmon.master = test_sys.smccontroller_pipeline.slave
        else:
            test_sys.membus.master = test_sys.smccontroller_pipeline.slave

        test_sys.smccontroller_pipeline.master = test_sys.smccontroller.slave

        for i in xrange(N_TARG_PORT):
            if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
                test_sys.smccontroller.master = test_sys.Lmon[i].slave
                test_sys.Lmon[i].master = test_sys.seriallink[i].slave
            else:
                test_sys.smccontroller.master = test_sys.seriallink[i].slave

            test_sys.seriallink[i].master = test_sys.smcxbar.slave
            test_sys.seriallink[i].clk_domain = SrcClockDomain(clock=SERIALLINKS_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))

    # Standalone PIM Cluster Simulation
    ################################
    else:
        test_sys.Hmon = CommMonitor()
        if ( HMON_DUMP_ADDRESS == "TRUE" ):
            test_sys.Hmon.dump_addresses=True
            test_sys.Hmon.dump_file="m5out/hmon_addr_dump.txt"
        test_sys.pimbridge = Bridge(ranges=test_sys.mem_ranges, delay='0.01ns', req_size=32, resp_size=32 ) #, width=32)
        test_sys.membus.master = test_sys.pimbridge.slave
        test_sys.pimbridge.master = test_sys.Hmon.slave

        if ( MOVE_CLUSTER_TO_HOST == "FALSE" ):
            test_sys.Hmon.master = test_sys.smcxbar.slave

        else:
            ######################################
            ### MOVE_CLUSTER_TO_HOST == "TRUE" ###
            ######################################
            # test_sys.smccontroller_pipeline = Bridge(ranges=test_sys.mem_ranges,
            #     req_size=GEM5_SMCCONTROLLER_BUFFER_SIZE_REQ, resp_size=GEM5_SMCCONTROLLER_BUFFER_SIZE_RSP, delay=GEM5_SMCCONTROLLER_LATENCY )

            # # Serial Links
            # test_sys.seriallink =[ SerialLink(ranges = test_sys.mem_ranges, 
            #     req_size=(GEM5_LINK_BUFFER_SIZE_REQ),
            #     resp_size=(GEM5_LINK_BUFFER_SIZE_RSP),
            #     serialization_unit=SERIALLINKS_NUM_LANES_PER_LINK,
            #     delay=GEM5_LINK_STATIC_LATENCY) for i in xrange(N_TARG_PORT)]

            # Serial Links
            test_sys.seriallink =[ Bridge(ranges = test_sys.mem_ranges, 
                req_size=(GEM5_LINK_BUFFER_SIZE_REQ),
                resp_size=(GEM5_LINK_BUFFER_SIZE_RSP),
                serialization_enable=True,
                serialization_unit=SERIALLINKS_NUM_LANES_PER_LINK,
                delay=GEM5_LINK_STATIC_LATENCY) for i in xrange(N_TARG_PORT)]
               
            # SMC Controller
            test_sys.smccontroller=LoadDistributor( width=SERIALLINKS_NUM_LANES_PER_LINK * SERIALLINKS_NUM_LINKS/8, header_cycles=8 )
            test_sys.smccontroller.clk_domain = SrcClockDomain(clock=SERIALLINKS_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))

            # test_sys.smccontroller_pipeline.master = test_sys.smccontroller.slave

            for i in xrange(N_TARG_PORT):
                test_sys.smccontroller.master = test_sys.seriallink[i].slave
                test_sys.seriallink[i].master = test_sys.smcxbar.slave
                test_sys.seriallink[i].clk_domain = SrcClockDomain(clock=SERIALLINKS_CLOCK_FREQUENCY, voltage_domain = VoltageDomain(voltage = '1V'))

            # test_sys.Hmon.master = test_sys.smccontroller_pipeline.slave
            test_sys.Hmon.master = test_sys.smccontroller.slave

    # Configure memory system
    #*******************************************************
    if ( GEM5_MEMTYPE == "ethz_ModelsimIF" or GEM5_MEMTYPE == "ethz_modelsim_if" ):
        options.mem_channels = 1
    else:
        options.mem_channels = N_INIT_PORT

    ethz_config_mem(options, test_sys)
    if ( HAVE_PIM_DEVICE == "TRUE" ):
        ethz_config_pim(test_sys, options)

    ethz_print_val("Memory Type", type( test_sys.mem_ctrls[0] ));
    if ( GEM5_MEMTYPE == "HMCVault" ):
        ethz_print_smc_config(test_sys, options);
    ethz_print_msg(colors.OKBLUE + "Memory " + str(type( test_sys.mem_ctrls[0] )) + " was configured successfully!" )

    # Add PIM ranges to all bridges in the memory system
    #  We do this in the last step to make sure that the memory configuration is ready
    if ( HAVE_PIM_DEVICE == "TRUE" and MOVE_PIM_TO_HOST == "FALSE" and HAVE_PIM_CLUSTER == "FALSE" ):
        test_sys.smccontroller_pipeline.ranges.append(pim_device_range);
        for i in xrange(N_TARG_PORT):
            test_sys.seriallink[i].ranges.append(pim_device_range);

# Copied from MemConfig.ethz_config_mem and modified
####################################
def ethz_config_mem(options, system):
    """
    Create the memory controllers based on the options and attach them.

    If requested, we make a multi-channel configuration of the
    selected memory controller class by creating multiple instances of
    the specific class. The individual controllers have their
    parameters set such that the address range is interleaved between
    them.
    """
    nbr_mem_ctrls = options.mem_channels
    import math
    from m5.util import fatal
    intlv_bits = int(math.log(nbr_mem_ctrls, 2))
    if 2 ** intlv_bits != nbr_mem_ctrls:
        fatal("Number of memory channels must be a power of 2")

    cls = MemConfig.get(options.mem_type)
    mem_ctrls = []

    # For every range (most systems will only have one), create an
    # array of controllers and set their parameters to match their
    # address mapping in the case of a DRAM
    for r in system.mem_ranges:
        for i in xrange(nbr_mem_ctrls):
            mem_ctrls.append(ethz_create_mem_ctrl(cls, r, i, nbr_mem_ctrls,
                                             intlv_bits,
                                             system.cache_line_size.value))

    system.mem_ctrls = mem_ctrls
    
    # Connect the controllers to the smcxbar
    for i in xrange(len(system.mem_ctrls)):
        system.mem_ctrls[i].port = system.smcxbar.master		# Modified by Erfan

    ethz_print_val("system.cache_line_size.value (bytes)", system.cache_line_size.value);
    
    if ( options.mem_type != "ethz_ModelsimIF" ):
        ethz_print_val("number of vaults", nbr_mem_ctrls);


# Copied from create_mem_ctrl and modified
####################################
def ethz_create_mem_ctrl(cls, r, i, nbr_mem_ctrls, intlv_bits, cache_line_size):
	"""
	Helper function for creating a single memoy controller from the given
	options.  This function is invoked multiple times in config_mem function
	to create an array of controllers.
	"""

	import math
	# The default behaviour is to interleave on cache line granularity
	cache_line_bit = int(math.log(cache_line_size, 2)) - 1
	intlv_low_bit = cache_line_bit

	# Create an instance so we can figure out the address
	# mapping and row-buffer size
	ctrl = cls()

	# Configure this instance of memory
	#*******************************************************
	if ( str(type( ctrl )) == "HMCVault" ):
		ethz_config_smc_vault(ctrl)
	elif ( str(type( ctrl )) == "ethz_ModelsimIF" ):
		ctrl.REPORT_PERIOD_ps = GEM5_REPORT_PERIOD_ps
		ctrl.SHORT_SLEEP_ns = GEM5_SHORT_SLEEP_ns
		ctrl.MAX_INFLIGHT_TRANS = MAX_INFLIGHT_TRANS
		ctrl.MAX_BUFFER_SIZE = GEM5_MAX_BUFFER_SIZE
		ctrl.SYNCH_PERIOD_ps = GEM5_SYNCH_PERIOD_ps
		ctrl.LONG_SLEEP_ns = GEM5_LONG_SLEEP_ns
		ctrl.latency = '0ns';
		ctrl.bandwidth = '500GB/s';
	elif ( str(type( ctrl )) == "SimpleMemory" ):
		ctrl.latency = GEM5_SIMPLEMEMORY_LATENCY;
		ctrl.bandwidth = GEM5_SIMPLEMEMORY_BW;
	elif ( str(type( ctrl )) == "DRAMSim2" ):
		ethz_print_msg("Configuring DRAMSim2");
		ctrl.deviceConfigFile = "results/HMC.ini"
		ctrl.systemConfigFile = "results/HMC_System.ini"
		if ( DRAMSIM2_ENABLE_DEBUG == "TRUE" ):
			ctrl.enableDebug=True;
		else:
			ctrl.enableDebug=False;
			
		if ( DRAMSIM2_ENABLE_TIMESTAMP == "TRUE"):
			ctrl.enableTimeStamp=True;
		else:
			ctrl.enableTimeStamp=False;
	else:
		ethz_print_msg("Memory type is not supported, please add support for this memory in ethz_configs.py! [" + str(type(ctrl))+"]");
		exit()
    
	# Only do this for DRAMs
	if issubclass(cls, m5.objects.DRAMCtrl):
		# Inform each controller how many channels to account
		# for
		ctrl.channels = nbr_mem_ctrls

		# If the channel bits are appearing after the column
		# bits, we need to add the appropriate number of bits
		# for the row buffer size
		if ctrl.addr_mapping.value == 'RoRaBaChCo':
			# This computation only really needs to happen
			# once, but as we rely on having an instance we
			# end up having to repeat it for each and every
			# one
			rowbuffer_size = ctrl.device_rowbuffer_size.value * \
				ctrl.devices_per_rank.value

			intlv_low_bit = int(math.log(rowbuffer_size, 2)) - 1
			
			

	# We got all we need to configure the appropriate address
	# range
	ctrl.range = m5.objects.AddrRange(r.start, size = r.size(),
					intlvHighBit = \
						intlv_low_bit + intlv_bits,
					intlvBits = intlv_bits,
					intlvMatch = i)
	return ctrl

####################################
def ethz_perform_sanity_checks(system):
    ethz_print_msg("Sanity Checks ...")
    try:
        if ( HAVE_PIM_DEVICE == "TRUE" ):
            ethz_perform_sanity_checks_pim(system.pim_sys)
        system.smcxbar;
    except Exception,e:
        ethz_print_msg("Error: the system relies on the name of some objects such as pim_sys. Please make sure that they exist with correct names.")
        print(str(e));
        exit(1)
    # if ( HAVE_PIM_DEVICE == "TRUE" ):
    #     if ( str(system.pim_sys.cache_line_size) != str(system.cache_line_size) ):
    #         print "system.cache_line_size (" + str(system.cache_line_size) +") != pim_sys.cache_line_size (" + str(system.pim_sys.cache_line_size)+")";
    #         exit(1)

    if ( HAVE_LISTENERS != "TRUE" ):
        m5.disableAllListeners();
        ethz_print_msg("VNC was disabled")

    if ( ARCH != "NONE" ):
        ethz_print_val("system.machine_type", system.machine_type)


####################################
def ethz_perform_sanity_checks_pim(pim_sys):
    if ( HAVE_PIM_DEVICE == "TRUE" ):
        try:
            pim_sys.pimbus;
            pim_sys.pim_memory;
        except Exception,e:
            ethz_print_msg("Error: the system relies on the name of some objects such as pim_sys. Please make sure that they exist with correct names.")
            print(str(e));
            exit(1)
        ethz_print_val("pim_sys.machine_type", pim_sys.machine_type)


	
