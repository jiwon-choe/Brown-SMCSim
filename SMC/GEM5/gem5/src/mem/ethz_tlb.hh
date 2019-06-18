#ifndef __ETHZ_TLB_HH__
#define __ETHZ_TLB_HH__

#include "base/types.hh"
#include "mem/addr_mapper.hh"
#include "params/ethz_TLB.hh"
#include <fstream>
#include <iostream>
#include "sim/system.hh"
#include "sim/stats.hh"
#include "dev/ethz_dma.hh"
#include <deque>

using namespace std;


class ethz_TLB : public RangeAddrMapper
{
public:
    ethz_TLB(const ethz_TLBParams *p);

    /*
    Remap the address if the rule is present in the TLB
    Otherwise return -1
    Notice: 0 means that address has not been found in the TLB
    */
    Addr remap(Addr addr);          // (modifies the internal state of the TLB)
    Addr remap_check(Addr addr);    // (Does not modify the internal state of the TLB)
    Addr slice_vend(Addr addr);  // (Does not modify the internal state of the TLB) - Returns the end vaddress in current slice
    Addr slice_vstart(Addr addr);  // (Does not modify the internal state of the TLB) - Returns the start vaddress in current slice
    Addr check_hmc_command(PacketPtr pkt);  // Check if this packet is an atomic hmc command

    /*
    Refill the rules in the TLB from the Slice Table
    */
    bool refillTLB_timing(Addr addr); // isDMA is only used by the DMA
    bool refillTLB_atomic(Addr addr);
    bool refillTLB_functional(Addr addr);
    bool pending_refill(Addr addr); // Is there a refil for this address pending?
    bool remove_pending_refill(Addr addr); // Remove an address from pending refill rule, return success


    // Override receive functions
    virtual void recvFunctional(PacketPtr pkt);
    virtual void recvFunctionalSnoop(PacketPtr pkt);
    virtual Tick recvAtomic(PacketPtr pkt);
    virtual Tick recvAtomicSnoop(PacketPtr pkt);
    virtual bool recvTimingReq(PacketPtr pkt);
    virtual bool recvTimingResp(PacketPtr pkt);
    virtual void recvTimingSnoopReq(PacketPtr pkt);
    virtual bool recvTimingSnoopResp(PacketPtr pkt);

    unsigned long PIM_SLICETABLE_PTR;     // A physical pointer to the slice-table (updated automatically)
	unsigned long PIM_SLICECOUNT;         // The number of slices (updated automatically from ethz_PIMMemory)
    unsigned long PIM_SLICEVSTART;        // Range start (virtual address) (updated automatically)
    unsigned OS_PAGE_SHIFT;               // Page shift of the guest OS running on ARM

    /**
     * Register Statistics
     */
    virtual void regStats();

    /** Number of total TLB misses */
    Stats::Scalar TLBAccess;
    Stats::Scalar TLBDynamicAccess;
    Stats::Scalar TLBMiss;
    Stats::Formula TLBHitRate;
    Stats::Formula TLBDynamicHitRate;


private:
    // Update TLB Rules (LRU)
    void updateTLB(unsigned long vaddr, unsigned long paddr, unsigned long size);
    int TLB_SIZE;
    bool DUMP_ADDRESS;
    bool IDEAL_REFILL;
    ofstream* DUMP_FILE;

    void printTLB();
    /*
    To implement the ideal LRU algorithm, we need the Tick which each rule has been
    used last
    */
    std::vector<Tick> lastUsed;
    std::vector<Addr> pending_refills; // Multiple pending addresses refill the rule for

    unsigned long int PIM_DTLB_IDEAL_REFILL_REG; // Register for refill request from software
    unsigned long int HMC_ATOMIC_INCR;
    unsigned long int HMC_ATOMIC_IMIN;
    unsigned long int HMC_ATOMIC_FADD;
    unsigned long int HMC_OPERAND; // Address of the operand register
    unsigned long int hmc_operand; // Value of the operand
    System* pim_system;

    // ID of the current master
    MasterID masterId;

//    // To handle writes to internal registers, we should act as a memory and respond directly
//    std::deque<Packet> packetQueue;
//
//    /**
//     * Dequeue a response packet from our internal packet queue and move it to
//     * the slave port where it will be sent as soon as possible.
//     */
    void dequeue();
    EventWrapper<ethz_TLB, &ethz_TLB::dequeue> dequeueEvent;
    std::deque<PacketPtr> packetQueue;

    // Override the retry methods
    virtual void recvRetryMaster();
    virtual void recvRetrySlave();

    // bool masterPortBusy; // Because DMA and TLB are sharing the masterPort, we must make sure that they do not access it together

public:
    // bool retry_belongs_to_dma;    // If the request belongs to DMA and we are not successful in sending the packet, we must send back the retry to DMA and not to the host CPU
    ethz_DMA* dma;
    unsigned pim_arch;
};

#endif //__ETHZ_TLB_HH__
