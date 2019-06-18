### FULL SYSTEM SIMULATION

import optparse
import sys

#*****************************
import ethz_utils
from ethz_utils import *
#*****************************

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../common')
addToPath('../ruby')

import Ruby

from FSConfig import *
from SysPaths import *
from Benchmarks import *
import Simulation
import CacheConfig
import MemConfig
from Caches import *
import Options

#*****************************
import ethz_configs
from ethz_configs import *
import _gem5_params
from _gem5_params import *
#*****************************

import m5.stats
from m5.internal.stats import periodicStatDump

# Check if KVM support has been enabled, we might need to do VM
# configuration if that's the case.
have_kvm_support = 'BaseKvmCPU' in globals()
####################################################################
def is_kvm_cpu(cpu_class):
    return have_kvm_support and cpu_class != None and \
        issubclass(cpu_class, BaseKvmCPU)

####################################################################
def build_test_system(np):

    if buildEnv['TARGET_ISA'] == "alpha":
        test_sys = makeLinuxAlphaSystem(test_mem_mode, bm[0], options.ruby)
    elif buildEnv['TARGET_ISA'] == "mips":
        test_sys = makeLinuxMipsSystem(test_mem_mode, bm[0])
    elif buildEnv['TARGET_ISA'] == "sparc":
        test_sys = makeSparcSystem(test_mem_mode, bm[0])
    elif buildEnv['TARGET_ISA'] == "x86":
        test_sys = makeLinuxX86System(test_mem_mode, options.num_cpus, bm[0],
                options.ruby)
    elif buildEnv['TARGET_ISA'] == "arm":
        test_sys = makeArmSystem(test_mem_mode, options.machine_type,
                                 options.num_cpus, bm[0], options.dtb_filename,
                                 bare_metal=options.bare_metal)
        if options.enable_context_switch_stats_dump:
            test_sys.enable_context_switch_stats_dump = True
    else:
        fatal("Incapable of building %s full system!", buildEnv['TARGET_ISA'])

    # Create a top-level voltage domain
    test_sys.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    test_sys.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = test_sys.voltage_domain)

    # Create a CPU voltage domain
    test_sys.cpu_voltage_domain = VoltageDomain()

    # Create a source clock for the CPUs and set the clock period
    test_sys.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                             voltage_domain =
                                             test_sys.cpu_voltage_domain)

    if options.kernel is not None:
        test_sys.kernel = binary(options.kernel)

    if options.script is not None:
        test_sys.readfile = options.script

    if options.lpae:
        test_sys.have_lpae = True

    if options.virtualisation:
        test_sys.have_virtualization = True

    test_sys.init_param = options.init_param

    # For now, assign all the CPUs to the same clock domain
    test_sys.cpu = [TestCPUClass(clk_domain=test_sys.cpu_clk_domain, cpu_id=i)
                    for i in xrange(np)]

    if is_kvm_cpu(TestCPUClass) or is_kvm_cpu(FutureClass):
        test_sys.vm = KvmVM()

    if options.ruby:
        # Check for timing mode because ruby does not support atomic accesses
        if not (options.cpu_type == "detailed" or options.cpu_type == "timing"):
            print >> sys.stderr, "Ruby requires TimingSimpleCPU or O3CPU!!"
            sys.exit(1)

        Ruby.create_system(options, test_sys, test_sys.iobus, test_sys._dma_ports)

        # Create a seperate clock domain for Ruby
        test_sys.ruby.clk_domain = SrcClockDomain(clock = options.ruby_clock,
                                        voltage_domain = test_sys.voltage_domain)

        for (i, cpu) in enumerate(test_sys.cpu):
            #
            # Tie the cpu ports to the correct ruby system ports
            #
            cpu.clk_domain = test_sys.cpu_clk_domain
            cpu.createThreads()
            cpu.createInterruptController()

            cpu.icache_port = test_sys.ruby._cpu_ports[i].slave
            cpu.dcache_port = test_sys.ruby._cpu_ports[i].slave

            if buildEnv['TARGET_ISA'] == "x86":
                cpu.itb.walker.port = test_sys.ruby._cpu_ports[i].slave
                cpu.dtb.walker.port = test_sys.ruby._cpu_ports[i].slave

                cpu.interrupts.pio = test_sys.ruby._cpu_ports[i].master
                cpu.interrupts.int_master = test_sys.ruby._cpu_ports[i].slave
                cpu.interrupts.int_slave = test_sys.ruby._cpu_ports[i].master

            test_sys.ruby._cpu_ports[i].access_phys_mem = True

        # Create the appropriate memory controllers
        # and connect them to the IO bus
        test_sys.mem_ctrls = [TestMemClass(range = r) for r in test_sys.mem_ranges]
        for i in xrange(len(test_sys.mem_ctrls)):
            test_sys.mem_ctrls[i].port = test_sys.iobus.master

    else:
        if options.caches or options.l2cache:
            # By default the IOCache runs at the system clock
            test_sys.iocache = IOCache(addr_ranges = test_sys.mem_ranges)
            test_sys.iocache.cpu_side = test_sys.iobus.master
            test_sys.iocache.mem_side = test_sys.membus.slave
        else:
            test_sys.iobridge = Bridge(delay='50ns', ranges = test_sys.mem_ranges)
            test_sys.iobridge.slave = test_sys.iobus.master
            test_sys.iobridge.master = test_sys.membus.slave

        # Sanity check
        if options.fastmem:
            if TestCPUClass != AtomicSimpleCPU:
                fatal("Fastmem can only be used with atomic CPU!")
            if (options.caches or options.l2cache):
                fatal("You cannot use fastmem in combination with caches!")

        for i in xrange(np):
            if options.fastmem:
                test_sys.cpu[i].fastmem = True
            if options.checker:
                test_sys.cpu[i].addCheckerCpu()
            test_sys.cpu[i].createThreads()

        CacheConfig.config_cache(options, test_sys)
        
	ethz_config_memory(test_sys, options);

    return test_sys
####################################################################

# Add options
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)

# Add the ruby specific and protocol specific options
if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# system under test can be any CPU
(TestCPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)

# Match the memories with the CPUs, based on the options for the test system
TestMemClass = Simulation.setMemClass(options)

if options.benchmark:
    try:
        bm = Benchmarks[options.benchmark]
    except KeyError:
        print "Error benchmark %s has not been defined." % options.benchmark
        print "Valid benchmarks are: %s" % DefinedBenchmarks
        sys.exit(1)
else:
    if options.dual:
        bm = [SysConfig(disk=options.disk_image, mem=options.mem_size),
              SysConfig(disk=options.disk_image, mem=options.mem_size)]
    else:
        bm = [SysConfig(disk=options.disk_image, mem=options.mem_size)]

np = options.num_cpus

test_sys = build_test_system(np)

if len(bm) == 2:
    ethz_print_msg("Error! not implemented");
elif len(bm) == 1:
    root = Root(full_system=True, system=test_sys)
else:
    print "Error I don't know how to create more than 2 systems."
    sys.exit(1)

if options.timesync:
    root.time_sync_enable = True

if options.frame_capture:
    VncServer.frame_capture = True
    
if ( GEM5_PERIODIC_STATS_DUMP == "TRUE" ):
	periodicStatDump(GEM5_PERIODIC_STATS_DUMP_PERIOD)

# This is to make bare_metal simulation work with ARMv8. If we don't make these
#  modifications, ARMv8 does not boot in bare_metal mode
if (options.bare_metal):
    test_sys.boot_loader = options.kernel
    test_sys.gic_cpu_addr = 0x00000001 # Dummy
    test_sys.flags_addr = 0x00000001 # Dummy
    test_sys.realview.nvmem = SimpleMemory(range = AddrRange(0, size = '1MB'))
    test_sys.realview.nvmem.port = test_sys.membus.master
	
ethz_perform_sanity_checks(test_sys)

# test_sys.pim_sys.cpu.commitWidth=1
# test_sys.pim_sys.cpu.decodeWidth=1
# test_sys.pim_sys.cpu.dispatchWidth=1
# test_sys.pim_sys.cpu.fetchWidth=1
# test_sys.pim_sys.cpu.issueWidth=1
# test_sys.pim_sys.cpu.renameWidth=1
# test_sys.pim_sys.cpu.squashWidth=1
# test_sys.pim_sys.cpu.wbWidth=1

# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth
# print test_sys.pim_sys.cpu.issueWidth

Simulation.setWorkCountOptions(test_sys, options)
Simulation.run(options, root, test_sys, FutureClass)
