from m5.params import *
from AddrMapper import *

class ethz_TLB(RangeAddrMapper):
    type = 'ethz_TLB'
    cxx_header = "mem/ethz_tlb.hh"

    TLB_SIZE = Param.Int(4, "Number of rules in the TLB");
    OS_PAGE_SHIFT = Param.Int(0, "Page Shift of the guest OS (usually 12)");
    DUMP_ADDRESS = Param.Bool(False, "Dump Accessed addresses to file");
    IDEAL_REFILL = Param.Bool(False, "Ideal refill");
    PIM_DTLB_IDEAL_REFILL_REG = Param.Int(0, "Register for the ideal TLB prefetcher");
    HMC_ATOMIC_INCR = Param.Int(0, "Register for HMC atomic commands");
    HMC_ATOMIC_IMIN = Param.Int(0, "Register for HMC atomic commands");
    HMC_ATOMIC_FADD = Param.Int(0, "Register for HMC atomic commands");
    HMC_OPERAND = Param.Int(0, "Register as an HMC atomic operand");
    pim_arch = Param.Int(32, "Determine whether PIM is a 32b (ARMv7-a) or a 64b (ARMv8-a) processor");
