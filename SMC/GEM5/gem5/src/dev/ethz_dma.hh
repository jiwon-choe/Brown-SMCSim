#ifndef __DEV_ETHZ_DMA_HH__
#define __DEV_ETHZ_DMA_HH__

#include <fstream>
#include <string>
#include <queue>

#include "dev/dma_device.hh"
#include "params/ethz_DMA.hh"
#include "sim/serialize.hh"
#include "mem/ethz_tlb.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"

#define DMA_STATE_IDLE 0
#define DMA_STATE_TLBREFILL 1
#define DMA_STATE_TRANSFER 2

//#define ETHZ_DEBUG_DMA

class ethz_PIMMemory;

/*
This DMA is designed to be used with PIM
It works with PIM's virtual address and its VECTOR registers
*/
class ethz_DMA: public DmaDevice
{
  private:

    //////////////////////////////
    // Pending requests to be done
    class DMARequest
    {
    public:
        /* Input Parameter */
        uint8_t* vreg_ptr;                  // Pointer to the vector
        unsigned long mem_vstart;           // Virtual start address of the memory location
        unsigned long mem_vend;             // Virtual end address of the memory location
        uint8_t dma_resource;              // Which DMA resource is used by this request
        bool RW;                            // true: (R) MEM-->V    false: (W) V-->MEM

        /* Internal Parameters */
        vector<unsigned long> mem_pstart;   // pstart addresses of injected transactions
        vector<unsigned long> mem_pend;     // pend addresses of injected transactions
        unsigned long bytes_expected;       // Expected number of bytes to transfer
        unsigned long bytes_completed;      // Number of bytes transferred up to now
        bool issued;

        /* Methods */
        DMARequest(uint8_t*vreg, unsigned long start, unsigned long num_bytes, bool dir, uint8_t dma_res)
        {
            vreg_ptr=vreg;
            dma_resource = dma_res;
            mem_vstart = start;
            RW = dir;
            mem_vend =  mem_vstart+num_bytes-1;
            bytes_expected = num_bytes;
            bytes_completed = 0;
            issued = false;
            print("new request");
        }
        bool nextSubRequest(unsigned long vaddr)
        {
            assert(vaddr>= mem_vstart);
            if (vaddr >= mem_vend)
                return false;
            unsigned long diff = vaddr - mem_vstart;
            mem_vstart += diff;
            vreg_ptr += diff;
            print("next subrequest");
            return true;
        }
        void print(string msg)
        {
            #ifdef ETHZ_DEBUG_DMA
            cout << "[PIM-DMA(0x" << hex << (unsigned)dma_resource << ")]: MEM: [0x" << hex << mem_vstart << ",0x" << mem_vend << "](VA) " << (RW?"-R->":"<-W-") 
            << " VREG " << dec << " Expected:" << bytes_expected <<  "(B) Completed:" << bytes_completed << "(B)" << " {" << msg << "}" << endl;
            #endif
        }
    };
    //////////////////////////////
    vector<DMARequest*> requests;   // List of pending requests
    long int current_index;         // Index of current requst being served

  public:
    /* Request for DMA operations */
    void initiate_write(uint8_t* vreg_ptr, unsigned long dma_mem_vaddr, unsigned long dma_num_bytes, uint8_t dma_resource);
    void initiate_read (uint8_t* vreg_ptr, unsigned long dma_mem_vaddr, unsigned long dma_num_bytes, uint8_t dma_resource);
    void wait_on_resource(uint8_t res);

    // uint8_t resource_to_wait_for;
    //bool dirty_interrupt;
    /* A pointer to PIM's DTLB */
    ethz_TLB* dtlb;
    /* Is our simulation running in timing mode? */
    bool isTimingMode;
    unsigned clk_per_ps;
    /* 
    nextState() will try to serve the next request or partial request. If it does not succeed
    It will wait for somebody to call it (Propably a retry)
    */
    void nextState();
    unsigned current_state;
    bool next_state_scheduled;
    EventWrapper<ethz_DMA, &ethz_DMA::nextState> nextStateEvent;
    BaseCPU* pim_cpu;
    ThreadContext* tc; // Thread context of the processor
    ethz_PIMMemory* pim_mem;

  protected:

    static const int maxOutstandingDma  = 16;   // TODO: 16 deep FIFO of 64 bits

    /**
     * Event wrapper for dmaDone()
     *
     * This event calls pushes its this pointer onto the freeDoneEvent
     * vector and calls dmaDone() when triggered.
     */
    class DmaDoneEvent : public Event
    {
      private:
        ethz_DMA &obj;

      public:
        DmaDoneEvent(ethz_DMA *_obj)
            : Event(), obj(*_obj) {}

        void process() {
            obj.dmaDoneEventFree.push_back(this);
            obj.dmaDone();
        }

        const std::string name() const {
            return obj.name() + ".DmaDoneEvent";
        }
    };

    /** Number of pending dma reads */
    int dmaPendingNum;

    uint8_t dma_resources; // status of the dma resources (1:busy, 0: free)
    unsigned long int PIM_DMA_STATUS_ADDR;
    void update_status_register();

    /** Function to generate interrupt */
    // void generateInterrupt();

    /** start the dmas off after power is enabled */
    void startDma();

    /** DMA done event */
    void dmaDone();

    /**@{*/
    /**
     * All pre-allocated DMA done events
     *
     * This model preallocates maxOutstandingDma number of
     * DmaDoneEvents to avoid having to heap allocate every single
     * event when it is needed. In order to keep track of which events
     * are in flight and which are ready to be used, we use two
     * different vectors. dmaDoneEventAll contains <i>all</i>
     * DmaDoneEvents that the object may use, while dmaDoneEventFree
     * contains a list of currently <i>unused</i> events. When an
     * event needs to be scheduled, the last element of the
     * dmaDoneEventFree is used and removed from the list. When an
     * event fires, it is added to the end of the
     * dmaEventFreeList. dmaDoneEventAll is never used except for in
     * initialization and serialization.
     */
    std::vector<DmaDoneEvent> dmaDoneEventAll;

    /** Unused DMA done events that are ready to be scheduled */
    std::vector<DmaDoneEvent *> dmaDoneEventFree;
    /**@}*/

    /** Wrapper to create an event out of the interrupt */
    //EventWrapper<ethz_DMA, &ethz_DMA::generateInterrupt> intEvent;

    //bool enableCapture;

  public:
    typedef ethz_DMAParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }
    ethz_DMA(const Params *p);
    ~ethz_DMA();

    virtual Tick read(PacketPtr pkt);
    virtual Tick write(PacketPtr pkt);

    virtual void serialize(std::ostream &os);
    virtual void unserialize(Checkpoint *cp, const std::string &section);

    AddrRangeList getAddrRanges() const; // Erfan: Does nothing

    // Handle multiple subtransactions
    virtual void handle_packet( PacketPtr pkt, bool last_packet, unsigned tid );
};

#endif
