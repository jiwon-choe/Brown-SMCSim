#include "base/trace.hh"
#include "debug/ethz_TLB.hh"
#include "mem/ethz_tlb.hh"
#include "params/ethz_TLB.hh"
#include "sim/system.hh"
#include "ethz_pim_definitions.h"

// #define ETHZ_DEBUG_PIM_TLB

ethz_TLB *
ethz_TLBParams::create()
{
    return new ethz_TLB(this);
}

ethz_TLB::ethz_TLB(const ethz_TLBParams* p) :
    RangeAddrMapper(p),
    OS_PAGE_SHIFT(p->OS_PAGE_SHIFT),
    TLB_SIZE(p->TLB_SIZE),
    DUMP_ADDRESS(p->DUMP_ADDRESS),
    IDEAL_REFILL(p->IDEAL_REFILL),
    PIM_DTLB_IDEAL_REFILL_REG(p->PIM_DTLB_IDEAL_REFILL_REG),
    HMC_ATOMIC_INCR(p->HMC_ATOMIC_INCR),
    HMC_ATOMIC_IMIN(p->HMC_ATOMIC_IMIN),
    HMC_ATOMIC_FADD(p->HMC_ATOMIC_FADD),
    HMC_OPERAND(p->HMC_OPERAND),
    dequeueEvent(this),
    pim_arch(p->pim_arch)
{
    PIM_SLICETABLE_PTR = 0;
    PIM_SLICECOUNT = 0;
    TLBDynamicAccess = 0;
    hmc_operand = 0;
    //retry_belongs_to_dma = false;
    dma = NULL;
    pim_system = NULL;
    // masterPortBusy = 0;

    // for (int i = 0; i < originalRanges.size(); ++i)
    //    cout << "TLB:  " << name() << " RULE: " << originalRanges[i].to_string() << " ==> " << remappedRanges[i].to_string() << endl;

    vector<System *> slist = System::systemList;
    for ( auto it = slist.begin(); it != slist.end(); it++ )
    {
        if ( (*it)->name() == "system.pim_sys" )
            pim_system = (*it);
    }
    if ( pim_system == nullptr )
    {
        cout << "Warning: system.pim_sys not found, assuming standalone simulation of the PIM system" << endl;
        //STANDALONE_SIMULATION=true;
        pim_system = (*slist.begin());
        masterId = 0;
    }
    else
        masterId = pim_system->getMasterId(name());

    if ( DUMP_ADDRESS )
    {
        string n = "m5out/";
        n += name();
        n += ".addr.dump";
        DUMP_FILE = new ofstream(n);
    }

    lastUsed.push_back((Tick)-1);
}

Addr
ethz_TLB::remap_check(Addr addr)
{
    for (int i = 0; i < originalRanges.size(); ++i) {
        if (originalRanges[i].contains(addr)) {
            Addr offset = addr - originalRanges[i].start();
            return offset + remappedRanges[i].start();
        }
    }
    return -1;
}

Addr
ethz_TLB::slice_vend(Addr addr)
{
    for (int i = 0; i < originalRanges.size(); ++i) {
        if (originalRanges[i].contains(addr)) {
            Addr e = originalRanges[i].start() + originalRanges[i].size()-1;
            // cout << "ADDR=" << hex << addr << endl;
            // cout << "START=" << originalRanges[i].start() << endl;
            // cout << "END=" << e << dec << endl;
            return e;
        }
    }
    panic("This method should never fail: slice_vend() ");
}

Addr
ethz_TLB::slice_vstart(Addr addr)
{
    for (int i = 0; i < originalRanges.size(); ++i) {
        if (originalRanges[i].contains(addr)) {
            Addr e = originalRanges[i].start();
            // cout << "ADDR=" << hex << addr << endl;
            // cout << "START=" << originalRanges[i].start() << endl;
            // cout << "END=" << e << dec << endl;
            return e;
        }
    }
    panic("This method should never fail: slice_vstart() ");
}

Addr
ethz_TLB::remap(Addr addr)
{
    DPRINTF(ethz_TLB, "remap: addr 0x%x\n", addr );
    TLBAccess++;
	
    for (int i = 0; i < originalRanges.size(); ++i) {
        if (originalRanges[i].contains(addr)) {
            if ( i > 0)
            {
                lastUsed[i] = curTick(); // Update last used (LRU Algorithm)
                TLBDynamicAccess++;
            }
            Addr offset = addr - originalRanges[i].start();
            return offset + remappedRanges[i].start();
        }
    }

    /*
    We have three TLBs: ITLB, DTLB, STLB
    Among these, only DTLB needs a reference to the SliceTable
    Because the other two only access PIM's SPM
    */
    if ( PIM_SLICETABLE_PTR )
    {
        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << name() << " TLB Miss! Addr:" << hex << addr << dec << endl;
        #endif
        TLBMiss++;
        return -1;
    }
    else
    {
        cout << name() << " Unresolvable Address: 0x" << hex << addr << dec << endl;
        panic("TLB: Unresolvable address!");
    }
    return addr;
}

// Receive functions

void
ethz_TLB::recvFunctional(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();

    // A request from software for refilling the TLB
    if (PIM_DTLB_IDEAL_REFILL_REG!=0 && orig_addr == PIM_DTLB_IDEAL_REFILL_REG)
    {
        panic("Not implemented! (ethz_TLB::recvFunctional - DTLB Ideal Refill");
    }

    Addr remapped_addr = remap(orig_addr);
    if ( remapped_addr == (Addr)-1 )
    {
        panic("Not implemented yet:ethz_TLB::recvFunctional");
    }
    pkt->setAddr(remapped_addr );
    masterPort.sendFunctional(pkt);
    pkt->setAddr(orig_addr);
}

Addr
ethz_TLB::check_hmc_command(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    Addr remapped_addr = 0;

    if ( orig_addr == 0 ) // Ignore
        return 0;

    /* Atomic HMC Command */
    if (orig_addr == HMC_ATOMIC_INCR && pkt->isWrite())
    {
        assert(pkt->getSize() == pim_arch/8);
        if ( pim_arch == 32 ) // 32b PIM
        {
            uint32_t* c = pkt->getPtr<uint32_t>();
            remapped_addr = *c;
        }
        else  // 64b PIM
        {
            uint64_t* c = pkt->getPtr<uint64_t>();
            remapped_addr = *c;
        }
        assert(remapped_addr!=(Addr)-1);
        if ( remapped_addr == 0 )
            return 0;
        pkt->ATOMIC_HMC_COMMAND = HMC_CMD_INC;
        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "HMC Command = " << HMC_CMD_INC << " Remapped Addr = " << hex << remapped_addr << dec << endl;
        #endif
    }
    else
    /* Atomic HMC Command */
    if (orig_addr == HMC_ATOMIC_FADD && pkt->isWrite())
    {
        assert(pkt->getSize() == pim_arch/8);
        if ( pim_arch == 32 ) // 32b PIM
        {
            uint32_t* c = pkt->getPtr<uint32_t>();
            remapped_addr = *c;
        }
        else  // 64b PIM
        {
            uint64_t* c = pkt->getPtr<uint64_t>();
            remapped_addr = *c;
        }
        assert(remapped_addr!=(Addr)-1);
        if ( remapped_addr == 0 )
            return 0;
        pkt->ATOMIC_HMC_COMMAND = HMC_CMD_FADD;
        pkt->ATOMIC_HMC_OPERAND = hmc_operand;

        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "HMC Command = " << HMC_CMD_FADD << " Remapped Addr = " << hex << remapped_addr << dec << endl;
        #endif
    }
    else
    /* Atomic HMC Command */
    if (orig_addr == HMC_ATOMIC_IMIN && pkt->isWrite())
    {
        assert(pkt->getSize() == pim_arch/8);
        if ( pim_arch == 32 ) // 32b PIM
        {
            uint32_t* c = pkt->getPtr<uint32_t>();
            remapped_addr = *c;
        }
        else  // 64b PIM
        {
            uint64_t* c = pkt->getPtr<uint64_t>();
            remapped_addr = *c;
        }
        assert(remapped_addr!=(Addr)-1);
        if ( remapped_addr == 0 )
            return 0;
        pkt->ATOMIC_HMC_COMMAND = HMC_CMD_MIN;
        pkt->ATOMIC_HMC_OPERAND = hmc_operand;

        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "HMC Command = " << HMC_CMD_MIN << " Remapped Addr = " << hex << remapped_addr << dec << endl;
        #endif
    }

    /* Atomic HMC Operand */
    if (orig_addr == HMC_OPERAND && pkt->isWrite())
    {
        //assert(pkt->getSize() == 4); // Currently, this register is used only as a float operand which is always 4 bytes
        uint32_t* c = pkt->getPtr<uint32_t>();
        hmc_operand= *c;
        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << " <HMC_Operand int=" << hmc_operand << " float=" << *(float*)&hmc_operand << ">" << endl;
        #endif
    }

    return remapped_addr;
}


Tick
ethz_TLB::recvAtomic(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();

    // A request from software for refilling the TLB
    if (PIM_DTLB_IDEAL_REFILL_REG!=0 && orig_addr == PIM_DTLB_IDEAL_REFILL_REG)
    {
        assert(pkt->getSize() == pim_arch/8);
        assert(pkt->isWrite());
        Addr a = 0;
        if ( pim_arch == 32 ) // 32b PIM
        {
            uint32_t* c = pkt->getPtr<uint32_t>();
            a = *c;
        }
        else  // 64b PIM
        {
            uint64_t* c = pkt->getPtr<uint64_t>();
            a = *c;
        }
        if ( a == 0 )
            cout << " <PIM_DTLB_IDEAL_REFILL=ignored>" << endl;
        else
        if ( a != 0 )
        {
            // cout << " <PIM_DTLB_IDEAL_REFILL=0x" << hex << a << dec << ">" << endl;
            if ( remap_check(a) == -1 )
                refillTLB_functional(a);
        }
        return 1; // TODO: Later: Latency of refill request
    }

    // Registers for Supporting HMC Commands
    Addr remapped_addr = 0;
    remapped_addr = check_hmc_command(pkt);
    if ( remapped_addr == 0 )
        remapped_addr = remap(orig_addr);
    else
        remapped_addr = remap(remapped_addr);

    if ( remapped_addr == (Addr)-1 )
    {
        refillTLB_atomic(orig_addr);
        remapped_addr = remap(orig_addr);
        if ( remapped_addr == (Addr)-1 )
            panic("TLB refill was not successful! (ethz_TLB::recvAtomic)");
    }
    // Dump the memory access pattern to file
    if ( DUMP_ADDRESS )
    {
        (*DUMP_FILE) << curTick() << " " << orig_addr << " " << pkt->getSize() << " ";
        if (pkt-> isWrite())
            (*DUMP_FILE) << "W" << endl;
        else
            (*DUMP_FILE) << "R" << endl;
    }

    pkt->setAddr(remapped_addr);
    Tick ret_tick =  masterPort.sendAtomic(pkt);
    pkt->setAddr(orig_addr);
    return ret_tick;
}

bool
ethz_TLB::recvTimingReq(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();

    /*******************************************
    A request from software for refilling the TLB
    ********************************************/
    if (PIM_DTLB_IDEAL_REFILL_REG!=0 && orig_addr == PIM_DTLB_IDEAL_REFILL_REG)
    {
        pkt->firstWordDelay = pkt->lastWordDelay = 0;
        assert(pkt->isWrite());
        assert(pkt->getSize() == pim_arch/8);
        if (pkt->needsResponse()) {
            pkt->makeResponse();
        }
        else
            panic("Only WRITE should happen to this register");

        /*
        Since TLB is directly connected to the processor, we assume that a retry
        is never required
        */
        packetQueue.push_back(pkt);
        schedule(dequeueEvent, curTick()+1);

        // Do the ideal refilling
        Addr a = 0;
        if (pim_arch == 32)
        {
            uint32_t* c = pkt->getPtr<uint32_t>();
            a = *c;
        }
        else
        {
            uint64_t* c = pkt->getPtr<uint64_t>();
            a = *c;
        }
        if ( a == 0 )
            cout << " <PIM_DTLB_IDEAL_REFILL=ignored>" << endl;
        else
        if ( a != 0 )
        {
            // cout << " <PIM_DTLB_IDEAL_REFILL=0x" << hex << a << dec << ">" << endl;
            if ( remap_check(a) == -1 )
                refillTLB_functional(a);
        }
        return true;       
    }
    bool needsResponse = pkt->needsResponse();
    bool memInhibitAsserted = pkt->memInhibitAsserted();
    //cout << "ethz_TLB::recvTimingReq 0x" << hex << pkt->getAddr() << " IS_REMAPPED:" << pkt->IS_REMAPPED << " CMD:" << pkt->cmdString()<< endl;

    /******************************************
    Normal packet which should be remapped here
    *******************************************/
    if ( ! pkt->IS_REMAPPED ) // Erfan: to fix the problem of overlapping address ranges between input and output
    {
        // Registers for Supporting HMC Commands
        Addr remapped_addr = 0;
        remapped_addr = check_hmc_command(pkt);
        if ( remapped_addr != 0 ) // This means that an HMC command has been received, so we must remap original address
            orig_addr = remapped_addr;
        remapped_addr = remap(orig_addr);

        if ( remapped_addr == (Addr)-1 )
        {
            if (IDEAL_REFILL)
            {
                refillTLB_functional(orig_addr);
                remapped_addr = remap(orig_addr);
                assert(remapped_addr != (Addr)-1);
            }
            else
            {
                //cout << "ethz_TLB::TLB Miss (Timing)" << hex << pkt->getAddr() << dec << " Pending Refills:" << pending_refills.size() << endl;
                refillTLB_timing(orig_addr);
                return false; // Not successful, retry later (I am responsible for sending the retry whenever the data is ready)
            }
        }

        // Dump the memory access pattern to file
        if ( DUMP_ADDRESS )
        {
            (*DUMP_FILE) << curTick() << " " << orig_addr << " " << pkt->getSize() << " ";
            if (pkt-> isWrite())
                (*DUMP_FILE) << "W" << endl;
            else
                (*DUMP_FILE) << "R" << endl;
        }
        
        pkt->setAddr(remapped_addr);
        pkt->IS_REMAPPED = true;
    }
    if (needsResponse && !memInhibitAsserted) {
        pkt->pushSenderState(new AddrMapperSenderState(orig_addr));
    }

    // Attempt to send the packet (always succeeds for inhibited
    // packets)
    bool successful = false;
    // if ( ! masterPortBusy )
    // {
    successful = masterPort.sendTimingReq(pkt);
    //     masterPortBusy = ! successful;
    // }
    //cout << "[MASTER PORT BUSY] = " << masterPortBusy << endl;

    // If not successful, restore the sender state
    if (!successful && needsResponse) {
        delete pkt->popSenderState();
    }

    return successful;
}

bool
ethz_TLB::recvTimingResp(PacketPtr pkt)
{
    AddrMapperSenderState* receivedState =
        dynamic_cast<AddrMapperSenderState*>(pkt->senderState);

    // Restore initial sender state
    if (receivedState == NULL)  // This is a refill response packet
    {
        //panic("AddrMapper %s got a response without sender state\n", name());
        //cout << "refillTLB (timing): Response received" << endl;
        assert(pkt->getSize() == 3*pim_arch/8);
        bool success = remove_pending_refill(pkt->getAddr());
        if ( !success )
            panic("success = remove_pending_refill(pkt->getAddr()) failed!");
        unsigned long vaddr = (pim_arch==32)?
            (unsigned long)(pkt->getPtr<uint32_t>()[0]):    // 32b PIM
            (unsigned long)(pkt->getPtr<uint64_t>()[0]);    // 64b PIM
        unsigned long paddr = (pim_arch==32)?
            (unsigned long)(pkt->getPtr<uint32_t>()[1]):    // 32b PIM
            (unsigned long)(pkt->getPtr<uint64_t>()[1]);    // 64b PIM
        unsigned long size  = (pim_arch==32)?
            (unsigned long)(pkt->getPtr<uint32_t>()[2]):    // 32b PIM
            (unsigned long)(pkt->getPtr<uint64_t>()[2]);    // 64b PIM

        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "ethz_TLB::recvTimingResp (timing): Rule: vaddr=0x" << hex << vaddr << " paddr=0x" << paddr << " size=" << dec << size << " pending_refills" << pending_refills.size() << dec << endl;
        #endif
        updateTLB(vaddr, paddr, size);

        delete pkt; // Packet will delete its request and its data
        /*
        Now we must ask the master to retry its previous access
        */
        slavePort.sendRetry();

        return true;
    }

    Addr remapped_addr = pkt->getAddr();

    // Restore the state and address
    pkt->senderState = receivedState->predecessor;
    pkt->setAddr(receivedState->origAddr);

    // Attempt to send the packet
    bool successful = slavePort.sendTimingResp(pkt);

    // If packet successfully sent, delete the sender state, otherwise
    // restore state
    if (successful) {
        delete receivedState;
    } else {
        // Don't delete anything and let the packet look like we did
        // not touch it
        pkt->senderState = receivedState;
        pkt->setAddr(remapped_addr);
    }
    return successful;
}

// Coherence is not implemented in PIM

void
ethz_TLB::recvFunctionalSnoop(PacketPtr pkt)
{
    panic("Illegal packet received: recvFunctionalSnoop");
}

Tick
ethz_TLB::recvAtomicSnoop(PacketPtr pkt)
{
    panic("Illegal packet received: recvAtomicSnoop");
    return 0;
}

void
ethz_TLB::recvTimingSnoopReq(PacketPtr pkt)
{
    panic("Illegal packet received: recvTimingSnoopReq");
}

bool
ethz_TLB::recvTimingSnoopResp(PacketPtr pkt)
{
    panic("Illegal packet received: recvTimingSnoopResp");
}

bool ethz_TLB::pending_refill(Addr addr)
{
    // Check if a request for this rule is already in flight:
    std::vector<Addr>::iterator it;
    it = std::find(pending_refills.begin(), pending_refills.end(), addr);
    if ( it != pending_refills.end() ) // found
    {
        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "ethz_TLB::pending_refill = true  address: " << hex << *it << dec << endl;
        #endif
        return true;
    }
    return false;
}

bool ethz_TLB::remove_pending_refill(Addr addr)
{
    // Check if a request for this rule is already in flight:
    std::vector<Addr>::iterator it;
    it = std::find(pending_refills.begin(), pending_refills.end(), addr);
    if ( it != pending_refills.end() ) // found
    {
        pending_refills.erase(it);
        return true;
    }
    return false; // Not found in the pending list
}

bool ethz_TLB::refillTLB_timing(Addr addr)
{
    int length = 3*pim_arch/8; // Each slice has 3 entries
    assert( addr >= PIM_SLICEVSTART );
    unsigned long index = ((addr - PIM_SLICEVSTART) >> OS_PAGE_SHIFT)*length;
    #ifdef ETHZ_DEBUG_PIM_TLB
    cout << "ethz_TLB::refillTLB (timing): Requesting rule for address: 0x" << hex << addr << " Index:" << dec << index << endl;
    #endif

    Request::Flags flags = 0;

    // Check if a request for this rule is already in flight:
    if ( pending_refill(PIM_SLICETABLE_PTR+index) )
    {
        cout << "ethz_TLB: Request is already issued. Nothing to do (addr=0x" << hex << addr << dec << ")" << endl;
        return true;
    }

    pending_refills.push_back(PIM_SLICETABLE_PTR+index);
    Request *req = new Request(PIM_SLICETABLE_PTR+index, length, flags, masterId); // Each slice is 3 words
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->setSrc(0);
    uint8_t* empty = new uint8_t[length];
    *empty = 0;

    pkt->dataDynamicArray(empty);

    bool successful = false;
    // if ( ! masterPortBusy )
    // {
    successful = masterPort.sendTimingReq(pkt);
    //     masterPortBusy = ! successful;
    // }
    //cout << "[MASTER PORT BUSY] = " << masterPortBusy << endl;
    if ( ! successful )
    {
        /*
        We were not successful in sending a refill request packet.
        We must wait until the original master retries its request
        Then, we will send another refill request

        Notice: if DMA was the source of this packet, it will retry
        on its next clock cycle
        */
        cout << "[STALL] refillTLB_timing.sendTimingReq was not successful" << endl;
        pending_refills.pop_back();
        delete pkt; // Packet will delete its request and its data
        return false;
    }
    return true;
}

bool ethz_TLB::refillTLB_atomic(Addr addr)
{
    int length = 3*pim_arch/8; // Each slice has 3 entries
    assert( addr >= PIM_SLICEVSTART );
    unsigned long index = ((addr - PIM_SLICEVSTART) >> OS_PAGE_SHIFT)*length;
    #ifdef ETHZ_DEBUG_PIM_TLB
    cout << "ethz_TLB::refillTLB (atomic): Refilling rule for address: 0x" << hex << addr << " Index:" << dec << index << endl;
    #endif

    bool status = pending_refill(PIM_SLICETABLE_PTR+index); // atomic and timing cannot coexist!
    if ( status )
        panic("refillTLB_atomic: atomic and timing cannot coexist!");

    Request::Flags flags = 0;
    Request *req = new Request(PIM_SLICETABLE_PTR+index, length, flags, masterId); // Each slice is 3 words
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->setSrc(0);
//  SequenceNumber* s = new SequenceNumber();
//  s->seqNum = seqNum;
//  pkt->pushSenderState(s);
    uint8_t* empty = new uint8_t[length];
    *empty = 0;

    pkt->dataDynamicArray(empty);
    masterPort.sendAtomic(pkt);

    //cout << "refillTLB (atomic): Response received" << endl;
    assert(pkt->getSize() == 3*pim_arch/8);
    unsigned long vaddr = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[0]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[0]);    // 64b PIM
    unsigned long paddr = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[1]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[1]);    // 64b PIM
    unsigned long size  = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[2]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[2]);    // 64b PIM

    #ifdef ETHZ_DEBUG_PIM_TLB
    cout << "ethz_TLB::refillTLB (atomic): Rule: vaddr=0x" << hex << vaddr << " paddr=0x" << paddr << " size=" << dec << size << endl;
    #endif
    updateTLB(vaddr, paddr, size);

    delete pkt; // Packet will delete its request and its data
    return true;
}

void ethz_TLB::updateTLB(unsigned long vaddr, unsigned long paddr, unsigned long size)
{
    if ( originalRanges.size() >= TLB_SIZE )
    {
        /* 
        Remove the LRU Rule
        The first rule should never be replaced
         */
        Tick min = lastUsed[1];
        long min_index = 1;
        for (int i=1; i<lastUsed.size(); i++)
        {
            if (min>lastUsed[i])
            {
                min = lastUsed[i];
                min_index = i;
            }
        }
        #ifdef ETHZ_DEBUG_PIM_TLB
        cout << "TLB:Replace[" << min_index << "]: Last Used:" << min << endl;
        #endif
        lastUsed.erase(lastUsed.begin()+min_index);
        originalRanges.erase(originalRanges.begin()+min_index);
        remappedRanges.erase(remappedRanges.begin()+min_index);
    }
    originalRanges.push_back(AddrRange(vaddr, vaddr + size-1));
    remappedRanges.push_back(AddrRange(paddr, paddr + size-1));
    lastUsed.push_back(curTick());

    #ifdef ETHZ_DEBUG_PIM_TLB
    //printTLB();
    #endif
}

void ethz_TLB::printTLB()
{
    // Print the whole TLB:
    cout << "---------------------------------" << endl;
    cout << "TLB: " << name() << endl;
    for (int i = 0; i < originalRanges.size(); ++i)
       cout << "  RULE: " << originalRanges[i].to_string() << " ==> " << remappedRanges[i].to_string() << "   Used @" << lastUsed[i] << endl;
    cout << "---------------------------------" << endl;
}

void
ethz_TLB::regStats()
{
    AddrMapper::regStats();
    using namespace Stats;

    TLBMiss
        .name(name() + ".tlb_miss")
        .desc("Number of TLB misses")
        ;

    TLBAccess
        .name(name() + ".tlb_access")
        .desc("Number of TLB accesses")
        ;

    TLBDynamicAccess
        .name(name() + ".tlb_dyn_access")
        .desc("Number of TLB accesses")
        ;

    TLBHitRate
        .name(name() + ".tlb_hit_rate")
        .desc("TLB hit rate")
        .precision(2);

    TLBHitRate = (1.0 - TLBMiss / TLBAccess) * 100.0;

    TLBDynamicHitRate
        .name(name() + ".tlb_dyn_hit_rate")
        .desc("TLB dynamic hit rate (hit rate for the dynamic rules only")
        .precision(2);

    TLBDynamicHitRate = (1.0 - TLBMiss / TLBDynamicAccess) * 100.0;
}

bool ethz_TLB::refillTLB_functional(Addr addr)
{
    int length = 3*pim_arch/8; // Each slice has 3 entries
    assert( addr >= PIM_SLICEVSTART );
    unsigned long index = ((addr - PIM_SLICEVSTART) >> OS_PAGE_SHIFT)*length;
    #ifdef ETHZ_DEBUG_PIM_TLB
    cout << "ethz_TLB::refillTLB (functional): Refilling rule for address: 0x" << hex << addr << " Index:" << dec << index << endl;
    #endif

    bool status = pending_refill(PIM_SLICETABLE_PTR+index); // This situation is not very bad, but we avoid it for now
    if ( status )
        panic("refillTLB_functional: this situation is not very bad, but we avoid it for now");

    Request::Flags flags = 0;
    Request *req = new Request(PIM_SLICETABLE_PTR+index, length, flags, masterId); // Each slice is 3 words
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->setSrc(0);
    uint8_t* empty = new uint8_t[length];
    *empty = 0;

    pkt->dataDynamicArray(empty);
    masterPort.sendFunctional(pkt);

    assert(pkt->getSize() == 3*pim_arch/8);
    unsigned long vaddr = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[0]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[0]);    // 64b PIM
    unsigned long paddr = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[1]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[1]);    // 64b PIM
    unsigned long size  = (pim_arch==32)?
        (unsigned long)(pkt->getPtr<uint32_t>()[2]):    // 32b PIM
        (unsigned long)(pkt->getPtr<uint64_t>()[2]);    // 64b PIM

    #ifdef ETHZ_DEBUG_PIM_TLB
    cout << "ethz_TLB::refillTLB (functional): Rule: vaddr=0x" << hex << vaddr << " paddr=0x" << paddr << " size=" << dec << size << endl;
    #endif
    updateTLB(vaddr, paddr, size);

    delete pkt; // Packet will delete its request and its data
    return true;
}

void
ethz_TLB::dequeue()
{
    assert(!packetQueue.empty());
    PacketPtr pkt = packetQueue.front();
    bool retry = !slavePort.sendTimingResp(pkt);

    if (!retry)
    {
        packetQueue.pop_front();
        assert (packetQueue.empty()); // Erfan
    }
    else
        panic("Response from TLB cannot be rejected by the processor! (No retry mechanism has been implemented)");
}

void ethz_TLB::recvRetryMaster()
{
    // //cout << "[MASTER PORT IS FREE]" << endl;
    // // masterPortBusy = false;
    // if (retry_belongs_to_dma)
    // {
    //     retry_belongs_to_dma = false;
    //     assert( dma->current_state == DMA_STATE_TLBREFILL ); // Not Idle
    //     if ( ! dma-> next_state_scheduled )
    //     {
    //         cout << "   [TLB: call DMA] :-)" << endl;
    //         dma->nextState();
    //     }
    //     else
    //         cout << "   [TLB: DMA is already active]" << endl;
    // }
    // else
        AddrMapper::recvRetryMaster();
}

void ethz_TLB::recvRetrySlave()
{
    AddrMapper::recvRetrySlave();
}
