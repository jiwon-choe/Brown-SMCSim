
# This Memory is devoted to the single-processor PIM

from m5.params import *
from SimpleMemory import *

class ethz_PIMMemory(SimpleMemory):
    type = 'ethz_PIMMemory'
    cxx_header = "mem/ethz_pim_memory.hh"
    PIM_ADDRESS_BASE = Param.Int(0, "Base address of the PIM device (Absolute Address)");
    PIM_DEBUG_ADDR = Param.Int(0, "PIM Debug Register (Absolute Address)");
    PIM_ERROR_ADDR = Param.Int(0, "PIM Error Register (Absolute Address)");
    PIM_INTR_ADDR = Param.Int(0, "PIM Error Register (Absolute Address)");
    PIM_COMMAND_ADDR = Param.Int(0, "PIM Command Register (Absolute Address)");
    PIM_STATUS_ADDR = Param.Int(0, "PIM Status Register (Absolute Address)");
    PIM_SLICETABLE_ADDR = Param.Int(0, "Register Storing the Address of the Slice Table (Absolute Address)");
    PIM_SLICECOUNT_ADDR = Param.Int(0, "Register Storing the Number of the Slices(Absolute Address)");
    PIM_SLICEVSTART_ADDR = Param.Int(0, "Register Storing the Virtual Address of the First Slice(Absolute Address)");
    PIM_M5_ADDR = Param.Int(0, "Register for sending commands to gem5 (debugging purposes)");
    PIM_M5_D1_ADDR = Param.Int(0, "Register for sending data to gem5 (debugging purposes)");
    PIM_M5_D2_ADDR = Param.Int(0, "Register for sending data to gem5 (debugging purposes)");
    PIM_VREG_ADDR = Param.Int(0, "Vector Register File");
    PIM_VREG_SIZE = Param.Int(0, "Size of the Vector Register File (Bytes)");
    PIM_SREG_ADDR = Param.Int(0, "Scalar Register File");
    PIM_SREG_COUNT = Param.Int(0, "Number of the Scalar Registers");
    PIM_DTLB_IDEAL_REFILL_ADDR = Param.Int(0, "Register for the ideal TLB prefetcher");
    SYSTEM_NUM_CPU = Param.Int(0, "Number of CPUs in the system (used for cache flush)");
    PIM_DMA_MEM_ADDR_ADDR = Param.Int(0, "Memory Virtual Address Register(Absolute Address)");
    PIM_DMA_SPM_ADDR_ADDR = Param.Int(0, "Local Physical Address");
    PIM_DMA_NUMBYTES_ADDR =  Param.Int(0, "Number of words to transfer using DMA (Absolute Address)");
    PIM_DMA_COMMAND_ADDR = Param.Int(0, "DMA Command Register(Absolute Address)");
    PIM_DMA_CLI_ADDR = Param.Int(0, "DMA Clear Interrupt Register(Absolute Address)");
    PIM_COPROCESSOR_CMD_ADDR = Param.Int(0, "Command to the PIM Coprocessor");
    pim_arch = Param.Int(32, "Determine whether PIM is a 32b (ARMv7-a) or a 64b (ARMv8-a) processor");
    HMC_ATOMIC_INCR_ADDR = Param.Int(0, "Sending atomic command to the HMC");
