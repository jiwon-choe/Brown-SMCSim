#TODO ERFAN
import m5
import m5.objects
from m5.objects import *
import inspect
from textwrap import  TextWrapper
import ethz_utils
from ethz_utils import *
import sys
import _gem5_params
from _gem5_params import *
import MemConfig
from SysPaths import *

##############################################################################
class PIMPlatform(Platform):
	type = 'RealView'
	cxx_header = "dev/arm/realview.hh"
	system = Param.System(Parent.any, "system")

##############################################################################
class PIMSystem(ArmSystem):
    pimbus = NoncoherentXBar(header_cycles=1, width=4)
    intrctrl = IntrControl()
    pim_platform = PIMPlatform()

##############################################################################
def makeStandalonePIMSystem(mem_mode, num_cpus = 1):
	self = PIMSystem()
	self.mem_mode = mem_mode	# Important
	ethz_print_val("mem_mode", colors.YELLOW+mem_mode);
	self.pimbus.use_default_range = True
	return self

##############################################################################
def build_pim_system(options, mem_mode, cpu_class):
	
    pim_sys = makeStandalonePIMSystem(mem_mode)

    # Create a top-level voltage domain
    pim_sys.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    pim_sys.clk_domain = SrcClockDomain(clock = PIM_CLOCK_FREQUENCY, voltage_domain = pim_sys.voltage_domain)
    
    # Create a CPU voltage domain
    pim_sys.cpu_voltage_domain = VoltageDomain()
    pim_sys.cache_line_size=SMC_BURST_SIZE_B # Needed for DMA transfers    

    # # Create a source clock for the CPUs and set the clock period
    # pim_sys.cpu_clk_domain = SrcClockDomain(clock = PIM_CLOCK_FREQUENCY,
    #                                          voltage_domain =
    #                                          pim_sys.cpu_voltage_domain)

    pim_sys.kernel = binary(GEM5_PIM_KERNEL)
    if ( mem_mode == "atomic" ):
        pim_sys.cpu = AtomicSimpleCPU(clk_domain=pim_sys.clk_domain, cpu_id=0)
    else:
        #pim_sys.cpu = TimingSimpleCPU(clk_domain=pim_sys.clk_domain, cpu_id=0)
        pim_sys.cpu = cpu_class(clk_domain=pim_sys.clk_domain, cpu_id=0)

    # ethz_print_val("Host Class", cpu_class);
    # ethz_print_val("PIM  Class", type(pim_sys.cpu));

    pim_sys.itlb = ethz_TLB()
    pim_sys.dtlb = ethz_TLB()
    pim_sys.stlb = ethz_TLB()

    pim_sys.dma = ethz_DMA()
    pim_sys.dma.dma = pim_sys.pimbus.slave
    pim_sys.dma.pio = pim_sys.pimbus.master
    pim_sys.dma.PIM_DMA_STATUS_ADDR = PIM_DMA_STATUS_ADDR

    if options.cpu_type == "arm_detailed":
        try:
            from O3_ARM_v7a import *
        except:
            print "arm_detailed is unavailable. Did you compile the O3 model?"
            sys.exit(1)

    pim_sys.cpu.createInterruptController()
    ### pim_sys.cpu.connectAllPorts(pim_sys.pimbus)	# Erfan: I have brought the contents of this function here ...

    # if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
    #     pim_sys.Dmon = CommMonitor()
    #     pim_sys.cpu.dcache_port = pim_sys.Dmon.slave
    #     pim_sys.Dmon.master = pim_sys.dtlb.slave
    # else:
    pim_sys.cpu.dcache_port = pim_sys.dtlb.slave

    pim_sys.dtlb.master = pim_sys.pimbus.slave

    # if ( GEM5_ENABLE_COMM_MONITORS == "TRUE" ):
    #     pim_sys.Imon= CommMonitor()
    #     pim_sys.cpu.icache_port = pim_sys.Imon.slave
    #     pim_sys.Imon.master = pim_sys.itlb.slave
    # else:
    pim_sys.cpu.icache_port = pim_sys.itlb.slave

    pim_sys.itlb.master = pim_sys.pimbus.slave
    # Default ranges for the TLBs
    pim_sys.itlb.original_ranges = AddrRange(0x00000000, size=PIM_ADDRESS_SIZE)
    pim_sys.dtlb.original_ranges = AddrRange(0x00000000, size=PIM_ADDRESS_SIZE)
    pim_sys.itlb.remapped_ranges = AddrRange(PIM_ADDRESS_BASE, size=PIM_ADDRESS_SIZE)
    pim_sys.dtlb.remapped_ranges = AddrRange(PIM_ADDRESS_BASE, size=PIM_ADDRESS_SIZE)
    pim_sys.dtlb.OS_PAGE_SHIFT = OS_PAGE_SHIFT   # Page shift of the guest OS
    pim_sys.dtlb.TLB_SIZE = PIM_DTLB_SIZE
    pim_sys.dtlb.DUMP_ADDRESS = True if (PIM_DTLB_DUMP_ADDRESS=="TRUE") else False
    pim_sys.dtlb.IDEAL_REFILL = True if (PIM_DTLB_DO_IDEAL_REFILL=="TRUE") else False

    # Pointer to the slice table
    pim_sys.stlb.original_ranges = AddrRange(0x00000000, size=PIM_ADDRESS_SIZE)
    pim_sys.stlb.remapped_ranges = AddrRange(PIM_ADDRESS_BASE, size=PIM_ADDRESS_SIZE)
    pim_sys.system_port = pim_sys.stlb.slave
    pim_sys.stlb.master = pim_sys.pimbus.slave
        
    r = AddrRange(PIM_ADDRESS_BASE, size=PIM_ADDRESS_SIZE)
    pim_sys.pim_memory = ethz_PIMMemory(range=r, latency=PIM_SPM_ACCESSTIME_ns, bandwidth=PIM_SPM_BW_Gbps )
    pim_sys.pim_memory.PIM_ADDRESS_BASE = PIM_ADDRESS_BASE
    pim_sys.pim_memory.PIM_DEBUG_ADDR = PIM_DEBUG_ADDR
    pim_sys.pim_memory.PIM_ERROR_ADDR = PIM_ERROR_ADDR
    pim_sys.pim_memory.PIM_INTR_ADDR = PIM_INTR_ADDR
    pim_sys.pim_memory.PIM_COMMAND_ADDR = PIM_COMMAND_ADDR
    pim_sys.pim_memory.PIM_STATUS_ADDR = PIM_STATUS_ADDR
    pim_sys.pim_memory.PIM_SLICETABLE_ADDR = PIM_SLICETABLE_ADDR
    pim_sys.pim_memory.PIM_SLICECOUNT_ADDR = PIM_SLICECOUNT_ADDR
    pim_sys.pim_memory.PIM_SLICEVSTART_ADDR = PIM_SLICEVSTART_ADDR
    pim_sys.pim_memory.PIM_M5_ADDR = PIM_M5_ADDR
    pim_sys.pim_memory.PIM_M5_D1_ADDR = PIM_M5_D1_ADDR
    pim_sys.pim_memory.PIM_M5_D2_ADDR = PIM_M5_D2_ADDR
    pim_sys.pim_memory.PIM_VREG_ADDR = PIM_VREG_ADDR
    pim_sys.pim_memory.PIM_VREG_SIZE = PIM_VREG_SIZE
    pim_sys.pim_memory.PIM_SREG_ADDR = PIM_SREG_ADDR
    pim_sys.pim_memory.PIM_SREG_COUNT = PIM_SREG_COUNT
    pim_sys.pim_memory.PIM_DMA_MEM_ADDR_ADDR = PIM_DMA_MEM_ADDR_ADDR
    pim_sys.pim_memory.PIM_DMA_SPM_ADDR_ADDR = PIM_DMA_SPM_ADDR_ADDR
    pim_sys.pim_memory.PIM_DMA_NUMBYTES_ADDR = PIM_DMA_NUMBYTES_ADDR
    pim_sys.pim_memory.PIM_DMA_COMMAND_ADDR = PIM_DMA_COMMAND_ADDR
    pim_sys.pim_memory.PIM_DMA_CLI_ADDR = PIM_DMA_CLI_ADDR
    pim_sys.pim_memory.PIM_DTLB_IDEAL_REFILL_ADDR = PIM_DTLB_IDEAL_REFILL_ADDR
    pim_sys.pim_memory.HMC_ATOMIC_INCR_ADDR = HMC_ATOMIC_INCR_ADDR
    pim_sys.pim_memory.PIM_COPROCESSOR_CMD_ADDR = PIM_COPROCESSOR_CMD_ADDR
    pim_sys.dtlb.PIM_DTLB_IDEAL_REFILL_REG = PIM_DTLB_IDEAL_REFILL_REG
    pim_sys.dtlb.HMC_ATOMIC_INCR = HMC_ATOMIC_INCR
    pim_sys.dtlb.HMC_ATOMIC_IMIN = HMC_ATOMIC_IMIN
    pim_sys.dtlb.HMC_ATOMIC_FADD = HMC_ATOMIC_FADD
    pim_sys.dtlb.HMC_OPERAND = HMC_OPERAND
    pim_sys.pim_memory.SYSTEM_NUM_CPU = GEM5_NUMCPU
    pim_sys.pim_memory.port = pim_sys.pimbus.default
    pim_sys.kernel_addr_check = False;		# Do not check for correct location of kernel (we do address translation in TLB)

    # Specify the architecture in the components
    if ( ARCH=="arm64"):
        pim_sys.pim_memory.pim_arch = 64
        pim_sys.dtlb.pim_arch = 64
    elif (ARCH=="arm"):
        pim_sys.pim_memory.pim_arch = 32
        pim_sys.dtlb.pim_arch = 32
    else:
        raise ValueError('ARCH is incorrect!');

    # Necessary options (specially for ARMv8)
    pim_sys.boot_loader = pim_sys.kernel
    pim_sys.gic_cpu_addr = 0x00000001 # Dummy
    pim_sys.flags_addr = 0x00000001 # Dummy
    pim_sys.machine_type = options.machine_type    # Erfan

    # pim_sys.realview.nvmem = SimpleMemory(range = AddrRange(0, size = '1MB'))
    # pim_sys.realview.nvmem.port = test_sys.membus.master


    return pim_sys
##############################################################################
