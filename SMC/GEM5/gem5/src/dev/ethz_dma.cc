/*
 * Copyright (c) 2010-2012 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: William Wang
 *          Ali Saidi
 */

#include "base/vnc/vncinput.hh"
#include "base/bitmap.hh"
#include "base/output.hh"
#include "base/trace.hh"
#include "dev/dma_device.hh"
#include "mem/ethz_pim_memory.hh"
#include "dev/ethz_dma.hh"
#include "debug/ethz_DMA.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"

#define MIN(a,b)  ((a>b)?b:a)

// clang complains about std::set being overloaded with Packet::set if
// we open up the entire namespace std
using std::vector;

// initialize clcd registers
ethz_DMA::ethz_DMA(const Params *p)
    : DmaDevice(p),
    dtlb(NULL),
    isTimingMode(false),
    clk_per_ps(p->clk_per_ps),
    nextStateEvent(this),
    dmaPendingNum(0),
    PIM_DMA_STATUS_ADDR(p->PIM_DMA_STATUS_ADDR),
    dmaDoneEventAll(maxOutstandingDma, this),
    dmaDoneEventFree(maxOutstandingDma)
    //intEvent(this)
{
    //resource_to_wait_for = 0;
    dma_resources = 0;
    //dirty_interrupt = false;
    pim_cpu = NULL;
    pim_mem = NULL;
    tc = NULL;
    current_index = -1;
    next_state_scheduled = false;
    assert (DMA_STATE_IDLE == 0 );
    current_state = DMA_STATE_IDLE;
    for (int i = 0; i < maxOutstandingDma; ++i)
        dmaDoneEventFree[i] = &dmaDoneEventAll[i];
    handle_packet_required = true;
}

ethz_DMA::~ethz_DMA()
{
}

void
ethz_DMA::nextState()
{
    #ifdef ETHZ_DEBUG_DMA
    //cout << "[DMA::State=" << current_state << "]" << endl;
    #endif
    bool is_active = true;
    unsigned long pstart;
    unsigned long pend, vend;
    DmaDoneEvent *event;

    // unsigned OS_PAGE_SIZE = 1 << dtlb->OS_PAGE_SHIFT;
    if ( requests.empty() )
    {
        current_state = DMA_STATE_IDLE;
        current_index = -1;
        is_active = false;
    }
    DMARequest* curr = NULL;
    if ( current_index != -1 )
        curr = requests[current_index];

    switch (current_state)
    {
        /*********************/
        case DMA_STATE_IDLE:
            if ( requests.empty() )
            {
                #ifdef ETHZ_DEBUG_DMA
                cout << "[Nothing to do] status=0x" << hex << (unsigned)dma_resources << dec << endl;
                #endif
                is_active = false;
            }
            else
            {
                current_index = -1;
                for ( int i=0; i< requests.size(); i++)
                {
                    if ( ! requests[i]->issued )
                    {
                        current_index = i;
                        break;
                    }
                }
                if ( current_index != -1 )
                    current_state = DMA_STATE_TLBREFILL;
                // else
                //     cout << "All requests are previously issued" << endl;
            }
        break;
        /*********************/
        case DMA_STATE_TLBREFILL:
            assert(curr);
            curr->issued = true; // This request will be issued
            if (curr->mem_vstart < curr->mem_vend)
            {
                curr->print("tlb refill");
                if (dtlb->remap_check(curr->mem_vstart) == -1) // Address is not present in TLB
                {
                    bool success = true;
                    if ( isTimingMode )
                        success = dtlb->refillTLB_functional(curr->mem_vstart);
                    else
                        success = dtlb->refillTLB_atomic(curr->mem_vstart);
                    if (!success)
                        panic("Not successful!");
                }
                current_state = DMA_STATE_TRANSFER;
            }
            else
                current_state = DMA_STATE_IDLE;
        break;
        /*********************/
        case DMA_STATE_TRANSFER:
            pstart = dtlb->remap_check(curr->mem_vstart);
            vend = MIN(dtlb->slice_vend(curr->mem_vstart), curr->mem_vend);
            pend   = dtlb->remap_check(vend);
            #ifdef ETHZ_DEBUG_DMA
            cout << "[PIM-DMA]: MEM: [0x" << hex << pstart << ",0x" << pend << "](PA) " << (curr->RW?"--->":"<---") << " VREG Bytes:" << dec << pend-pstart+1 << " (transfer)" << endl;
            #endif

            assert ( pend-pstart == vend - curr->mem_vstart );

            assert(!dmaDoneEventFree.empty());
            event = dmaDoneEventFree.back();
            dmaDoneEventFree.pop_back();
            assert(!event->scheduled());

            curr->mem_pstart.push_back(pstart);
            curr->mem_pend.push_back(pend);
            dmaPort.dmaAction((curr->RW?MemCmd::ReadReq:MemCmd::WriteReq), pstart, pend-pstart+1, event, curr->vreg_ptr, 0, Request::UNCACHEABLE);

            if ( curr->nextSubRequest(vend+1) )
                current_state = DMA_STATE_TLBREFILL;
            else
                current_state = DMA_STATE_IDLE;
        break;
        default:
            panic("Illegal state!");
    }
    /**********************************************************/
    // if ((dma_resources & resource_to_wait_for) == 0)
    // {
    //     if ( tc->status() == ThreadContext::Status::Suspended )
    //     {
    //         #ifdef ETHZ_DEBUG_DMA
    //         cout << "[PIM-DMA] wakeup cpu because of resource" << hex << (unsigned)resource_to_wait_for << " status=0x" << (unsigned)dma_resources << dec<<  endl;
    //         #endif
    //         //dirty_interrupt = true;
    //         pim_cpu->postInterrupt(ArmISA::INT_IRQ, 0);
    //         //pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
    //     }
    //     resource_to_wait_for = 0;
    // }

    if (is_active)
    {
        next_state_scheduled = true;
        schedule(nextStateEvent, curTick() + clk_per_ps);
    }
    else
    {
        next_state_scheduled = false;
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA] [NO RESCHEDULE] status=0x" << hex << (unsigned)dma_resources << dec<<  endl;
        cout << "PIM CPU Status = " <<
          ((tc->status() == ThreadContext::Status::Suspended)?"Suspended":
          (tc->status() == ThreadContext::Status::Active)?"Active": "Halted") << endl;
        #endif
        pim_cpu->postInterrupt(ArmISA::INT_IRQ, 0);
    }
}

void
ethz_DMA::wait_on_resource(uint8_t res)
{

    if ( res == 0xFF )
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA] Clear Interrupt --- Status: 0x" << (unsigned)dma_resources << dec << endl;
        #endif
        pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
        return;
    }
    panic("Illegal value");
// //    assert (resource_to_wait_for == 0);
//     if (dma_resources & res)
//     {
//         #ifdef ETHZ_DEBUG_DMA
//         cout << "[PIM-DMA] Wait on resource: 0x" << hex << (unsigned)res << " Status: 0x" << (unsigned)dma_resources << dec << endl;
//         #endif
// //        resource_to_wait_for=res;
//     }
}

void
ethz_DMA::initiate_read(uint8_t* vreg_ptr, unsigned long dma_mem_vaddr, unsigned long dma_num_bytes, uint8_t dma_resource)
{
    assert(dtlb);
    assert(dma_resource);
    // if ( dirty_interrupt)
    // {
    //     pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
    //     dirty_interrupt = false;
    // }

    #ifdef ETHZ_DEBUG_DMA
    cout << "[PIM-DMA] Requested resource: 0x" << hex << (unsigned)dma_resource << " Status: 0x" << (unsigned)dma_resources << dec << endl;
    #endif

    if (dma_resources & dma_resource)
    {
        cout << hex << "READ: dma_resources=" << (unsigned) dma_resources << "  dma_resource=" << (unsigned) dma_resource << dec << endl;
        panic("[PIM-DMA] READ: DMA resource is already busy!");
    }
    dma_resources |= dma_resource;
    update_status_register();
    requests.insert(requests.begin(), new DMARequest(vreg_ptr, dma_mem_vaddr, dma_num_bytes, true, dma_resource));

    if ( next_state_scheduled )
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA] DMA is already active ... " << endl;
        #endif
    }
    else
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA][INITIATE READ]" << endl;
        #endif
        next_state_scheduled = true;
        nextState();
    }
}

void
ethz_DMA::initiate_write(uint8_t* vreg_ptr, unsigned long dma_mem_vaddr, unsigned long dma_num_bytes, uint8_t dma_resource)
{
    assert(dtlb);
    assert(dma_resource);
    //pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
    #ifdef ETHZ_DEBUG_DMA
    cout << "[PIM-DMA] Requested resource: 0x" << hex << (unsigned)dma_resource << " Status: 0x" << (unsigned)dma_resources << dec << endl;
    #endif

    if (dma_resources & dma_resource)
    {
        cout << hex << "WRITE: dma_resources=" << (unsigned) dma_resources << "  dma_resource=" << (unsigned) dma_resource << dec << endl;
        panic("[PIM-DMA] WRITE: DMA resource is already busy!");
    }
    dma_resources |= dma_resource;
    update_status_register();
    requests.insert(requests.begin(), new DMARequest(vreg_ptr, dma_mem_vaddr, dma_num_bytes, false, dma_resource));

    if ( next_state_scheduled )
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA] DMA is already active ... " << endl;
        #endif
    }
    else
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "[PIM-DMA][INITIATE WRITE]" << endl;
        #endif
        next_state_scheduled = true;
        nextState();
    }
}

// read registers
Tick
ethz_DMA::read(PacketPtr pkt)
{
    panic("Not implemented!");
    return 0;
}

// write registers
Tick
ethz_DMA::write(PacketPtr pkt)
{
    panic("Not implemented!");
    return 0;
}

void
ethz_DMA::dmaDone()
{
    // Do nothing
    // DPRINTF(ethz_DMA, "DMA Done\n");
}

void
ethz_DMA::serialize(std::ostream &os)
{
    DPRINTF(ethz_DMA, "Serializing ethz DMA\n");
    SERIALIZE_SCALAR(dmaPendingNum);

    Tick int_event_time = 0;
    Tick read_event_time = 0;
    Tick fill_fifo_event_time = 0;
    // if (intEvent.scheduled())
    //     int_event_time = intEvent.when();

    SERIALIZE_SCALAR(read_event_time);
    SERIALIZE_SCALAR(fill_fifo_event_time);
    SERIALIZE_SCALAR(int_event_time);

    vector<Tick> dma_done_event_tick;
    dma_done_event_tick.resize(maxOutstandingDma);
    for (int x = 0; x < maxOutstandingDma; x++) {
        dma_done_event_tick[x] = dmaDoneEventAll[x].scheduled() ?
            dmaDoneEventAll[x].when() : 0;
    }
    arrayParamOut(os, "dma_done_event_tick", dma_done_event_tick);
}

void
ethz_DMA::unserialize(Checkpoint *cp, const std::string &section)
{
    DPRINTF(ethz_DMA, "Unserializing ARM ethz_DMA\n");
    UNSERIALIZE_SCALAR(dmaPendingNum);

    Tick int_event_time = 0;
    Tick read_event_time = 0;
    Tick fill_fifo_event_time = 0;

    UNSERIALIZE_SCALAR(read_event_time);
    UNSERIALIZE_SCALAR(fill_fifo_event_time);
    UNSERIALIZE_SCALAR(int_event_time);

    // if (int_event_time)
    //     schedule(intEvent, int_event_time);

    vector<Tick> dma_done_event_tick;
    dma_done_event_tick.resize(maxOutstandingDma);
    arrayParamIn(cp, section, "dma_done_event_tick", dma_done_event_tick);
    dmaDoneEventFree.clear();
    for (int x = 0; x < maxOutstandingDma; x++) {
        if (dma_done_event_tick[x])
            schedule(dmaDoneEventAll[x], dma_done_event_tick[x]);
        else
            dmaDoneEventFree.push_back(&dmaDoneEventAll[x]);
    }
    assert(maxOutstandingDma - dmaDoneEventFree.size() == dmaPendingNum);
}

void
ethz_DMA::handle_packet( PacketPtr pkt, bool last_packet, unsigned tid )
{
    /*
    Here a subrequest has been completed, so we just need to account for it
    There is nothing to do with the payload of this packet
    */
    DMARequest* curr = NULL;
    long int delete_index = -1;

    // Check if this response matches any of the inflight transactions
    for (int i=0; i<requests.size(); i++)
        for (int j=0; j<requests[i]->mem_pstart.size(); j++)
        {
            if ( requests[i]->mem_pstart[j] <= pkt->getAddr() && pkt->getAddr() <= requests[i]->mem_pend[j])
            {
                if ( curr )
                    panic("One response belongs to two different requets. (Overlap between ranges)");
                //cout << "Packet matched REQ[]" << i << endl;
                curr = requests[i];
                delete_index = i;
            }
        }
    assert(curr);
    assert(delete_index != -1);

    curr->bytes_completed+= pkt->getSize();
    curr->print("response received");
    assert( curr->bytes_completed <= curr->bytes_expected );
    if ( curr->bytes_completed == curr->bytes_expected )
    {
        #ifdef ETHZ_DEBUG_DMA
        cout << "Transfer completed: resource:0x" << hex << (unsigned) curr->dma_resource << dec <<endl;
        cout << "PIM CPU Status = " <<
          ((tc->status() == ThreadContext::Status::Suspended)?"Suspended":
          (tc->status() == ThreadContext::Status::Active)?"Active": "Halted") << endl;
        #endif

        // if ((dma_resources & resource_to_wait_for) == 0)
        // {
            if ( tc->status() == ThreadContext::Status::Suspended )
            {
                #ifdef ETHZ_DEBUG_DMA
                cout << "CPU woke up!" << endl;
                #endif
                //dirty_interrupt = true;
                pim_cpu->postInterrupt(ArmISA::INT_IRQ, 0);
                //pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
            }
        //     resource_to_wait_for = 0;
        // }

        assert(dma_resources & curr->dma_resource);
        dma_resources &= ~(curr->dma_resource);
        update_status_register();
        assert((dma_resources & curr->dma_resource) == 0);

        requests.erase(requests.begin()+delete_index);
        delete curr;
    }
}

void
ethz_DMA::update_status_register()
{
    assert(pim_mem);
    pim_mem->__write_mem_1byte(PIM_DMA_STATUS_ADDR, dma_resources);
}

AddrRangeList
ethz_DMA::getAddrRanges() const
{
    AddrRangeList ranges;
    return ranges; //Empty ranges
}

ethz_DMA *
ethz_DMAParams::create()
{
    return new ethz_DMA(this);
}


