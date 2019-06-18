/**
 * @file
 * ethz_PIMMemory declaration
 */

// This Memory is devoted to the single-processor PIM

#ifndef __ETHZ_PIM_MEMORY_HH__
#define __ETHZ_PIM_MEMORY_HH__

#include <deque>

#include "mem/simple_mem.hh"
#include "mem/port.hh"
#include "mem/ethz_tlb.hh"
#include "params/ethz_PIMMemory.hh"
#include "sim/system.hh"
#include "cpu/base.hh"

class ethz_DMA;

class ethz_PIMMemory : public SimpleMemory
{
public:
	ethz_PIMMemory(const ethz_PIMMemoryParams* p);

    // Read from our memory (ERFAN)
    unsigned long __read_mem_1word(unsigned long int address);
    void          __write_mem_1byte(unsigned long int address, uint8_t value);

protected:
	virtual Tick recvAtomic(PacketPtr pkt);
	virtual void recvFunctional(PacketPtr pkt);
	virtual bool recvTimingReq(PacketPtr pkt);
	virtual void init();
private:
	
	/* Simulation hack to perform the cache flush */
	void ask_host_to_flush_the_caches();

    unsigned long int PIM_ADDRESS_BASE;
	unsigned long int PIM_DEBUG_ADDR;
	unsigned long int PIM_ERROR_ADDR;
	unsigned long int PIM_INTR_ADDR;
	unsigned long int PIM_COMMAND_ADDR;
	unsigned long int PIM_STATUS_ADDR;
	unsigned long int PIM_SLICETABLE_ADDR;
	unsigned long int PIM_SLICECOUNT_ADDR;
	unsigned long int PIM_SLICEVSTART_ADDR;
	unsigned long int PIM_M5_ADDR;
	unsigned long int PIM_M5_D1_ADDR;
	unsigned long int PIM_M5_D2_ADDR;
    unsigned long int PIM_VREG_ADDR;
    unsigned long int PIM_VREG_SIZE;
    unsigned long int PIM_SREG_ADDR;
    unsigned long int PIM_SREG_COUNT;
    unsigned long int PIM_DTLB_IDEAL_REFILL_ADDR;
    unsigned long int PIM_DMA_MEM_ADDR_ADDR;
    unsigned long int PIM_DMA_SPM_ADDR_ADDR;
    unsigned long int PIM_DMA_NUMBYTES_ADDR;
    unsigned long int PIM_DMA_COMMAND_ADDR;
    unsigned long int PIM_DMA_CLI_ADDR;
    unsigned long int PIM_COPROCESSOR_CMD_ADDR;
	unsigned long int SYSTEM_NUM_CPU;
    unsigned pim_arch;
    unsigned long int HMC_ATOMIC_INCR_ADDR;

	System* pim_system;
	BaseCPU* pim_cpu;
	ethz_TLB* dtlb;	// Pointer to the DTLB inside PIM
    ethz_DMA* dma;  // Pointer to the DMA inside PIM

	Tick prev_time_stamp;
	char prev_time_stamp_id;

	// Hack: caches to be flushed
	vector<SimObject*> system_caches;

    unsigned long _dma_num_bytes;
    unsigned long _dma_mem_addr;
    unsigned long _dma_spm_addr;

    void run_coprocessor_32b(uint8_t cmd);
    void run_coprocessor_64b(uint8_t cmd);
};

#endif //__ETHZ_PIM_MEMORY_HH__
