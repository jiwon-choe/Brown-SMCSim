#ifndef __ETHZ_PIM_IF_HH__
#define __ETHZ_PIM_IF_HH__

#include <deque>

#include "base/types.hh"
#include "mem/mem_object.hh"
#include "params/ethz_PIMIF.hh"
#include "mem/request.hh"
#include "sim/system.hh"

#include <fstream>
using namespace std;

class ethz_PIMIF : public MemObject
{
  protected:
	  
     static ofstream ethz_pim_ifparamsfile; // ERFAN

    /**
     * A ethz_pim_if request state stores packets along with their sender
     * state and original source. It has enough information to also
     * restore the response once it comes back to the ethz_pim_if.
     */
    class RequestState : public Packet::SenderState
    {

      public:

        const PortID origSrc;

        RequestState(PortID orig_src) : origSrc(orig_src)
        { }

    };

    /**
     * A deferred packet stores a packet along with its scheduled
     * transmission time
     */
    class DeferredPacket
    {

      public:

        const Tick tick;
        const PacketPtr pkt;

        DeferredPacket(PacketPtr _pkt, Tick _tick) : tick(_tick), pkt(_pkt)
        { }
    };

    // Forward declaration to allow the slave port to have a pointer
    class ethz_PIMIFMasterPort;

    /**
     * The port on the side that receives requests and sends
     * responses. The slave port has a set of address ranges that it
     * is responsible for. The slave port also has a buffer for the
     * responses not yet sent.
     */
    class ethz_PIMIFSlavePort : public SlavePort
    {

      private:

        /** The ethz_pim_if to which this port belongs. */
        ethz_PIMIF& ethz_pim_if;

        /**
         * Master port on the other side of the ethz_pim_if.
         */
        ethz_PIMIFMasterPort& masterPort;

        /** Minimum request delay though this ethz_pim_if. */
        const Cycles delay;

        ///** Address ranges to pass through the ethz_pim_if */
        const AddrRangeList ranges;

        /**
         * Response packet queue. Response packets are held in this
         * queue for a specified delay to model the processing delay
         * of the ethz_pim_if. We use a deque as we need to iterate over
         * the items for functional accesses.
         */
        std::deque<DeferredPacket> transmitList;

        /** Counter to track the outstanding responses. */
        unsigned int outstandingResponses;

        /** If we should send a retry when space becomes available. */
        bool retryReq;

        /** Max queue size for reserved responses. */
        unsigned int respQueueLimit;

        /**
         * Is this side blocked from accepting new response packets.
         *
         * @return true if the reserved space has reached the set limit
         */
        bool respQueueFull() const;

        /**
         * Handle send event, scheduled when the packet at the head of
         * the response queue is ready to transmit (for timing
         * accesses only).
         */
        void trySendTiming();

        /** Send event for the response queue. */
        EventWrapper<ethz_PIMIFSlavePort,
                     &ethz_PIMIFSlavePort::trySendTiming> sendEvent;

      public:

        /**
         * Constructor for the ethz_PIMIFSlavePort.
         *
         * @param _name the port name including the owner
         * @param _ethz_pim_if the structural owner
         * @param _masterPort the master port on the other side of the ethz_pim_if
         * @param _delay the delay in cycles from receiving to sending
         * @param _resp_limit the size of the response queue
         */
        ethz_PIMIFSlavePort(const std::string& _name, ethz_PIMIF& _ethz_pim_if,
                        ethz_PIMIFMasterPort& _masterPort, Cycles _delay,
                        int _resp_limit, std::vector<AddrRange> _ranges);

        /**
         * Queue a response packet to be sent out later and also schedule
         * a send if necessary.
         *
         * @param pkt a response to send out after a delay
         * @param when tick when response packet should be sent
         */
        void schedTimingResp(PacketPtr pkt, Tick when);

        /**
         * Retry any stalled request that we have failed to accept at
         * an earlier point in time. This call will do nothing if no
         * request is waiting.
         */
        void retryStalledReq();

      protected:

        /** When receiving a timing request from the peer port,
            pass it to the ethz_pim_if. */
        bool recvTimingReq(PacketPtr pkt);

        /** When receiving a retry request from the peer port,
            pass it to the ethz_pim_if. */
        void recvRetry();

        /** When receiving a Atomic requestfrom the peer port,
            pass it to the ethz_pim_if. */
        Tick recvAtomic(PacketPtr pkt);

        /** When receiving a Functional request from the peer port,
            pass it to the ethz_pim_if. */
        void recvFunctional(PacketPtr pkt);

        /** When receiving a address range request the peer port,
            pass it to the ethz_pim_if. */
        AddrRangeList getAddrRanges() const;
    };


    /**
     * Port on the side that forwards requests and receives
     * responses. The master port has a buffer for the requests not
     * yet sent.
     */
    class ethz_PIMIFMasterPort : public MasterPort
    {

      private:

        /** The ethz_pim_if to which this port belongs. */
        ethz_PIMIF& ethz_pim_if;

        /**
         * The slave port on the other side of the ethz_pim_if.
         */
        ethz_PIMIFSlavePort& slavePort;

        /** Minimum delay though this ethz_pim_if. */
        const Cycles delay;

        /**
         * Request packet queue. Request packets are held in this
         * queue for a specified delay to model the processing delay
         * of the ethz_pim_if.  We use a deque as we need to iterate over
         * the items for functional accesses.
         */
        std::deque<DeferredPacket> transmitList;

        /** Max queue size for request packets */
        const unsigned int reqQueueLimit;

        /**
         * Handle send event, scheduled when the packet at the head of
         * the outbound queue is ready to transmit (for timing
         * accesses only).
         */
        void trySendTiming();

        /** Send event for the request queue. */
        EventWrapper<ethz_PIMIFMasterPort,
                     &ethz_PIMIFMasterPort::trySendTiming> sendEvent;

      public:

        /**
         * Constructor for the ethz_PIMIFMasterPort.
         *
         * @param _name the port name including the owner
         * @param _ethz_pim_if the structural owner
         * @param _slavePort the slave port on the other side of the ethz_pim_if
         * @param _delay the delay in cycles from receiving to sending
         * @param _req_limit the size of the request queue
         */
        ethz_PIMIFMasterPort(const std::string& _name, ethz_PIMIF& _ethz_pim_if,
                         ethz_PIMIFSlavePort& _slavePort, Cycles _delay,
                         int _req_limit);

        /**
         * Is this side blocked from accepting new request packets.
         *
         * @return true if the occupied space has reached the set limit
         */
        bool reqQueueFull() const;

        /**
         * Queue a request packet to be sent out later and also schedule
         * a send if necessary.
         *
         * @param pkt a request to send out after a delay
         * @param when tick when response packet should be sent
	 * Erfan changed return value to bool
         */
        bool schedTimingReq(PacketPtr pkt, Tick when);

        /**
         * Check a functional request against the packets in our
         * request queue.
         *
         * @param pkt packet to check against
         *
         * @return true if we find a match
         */
        bool checkFunctional(PacketPtr pkt);

      protected:

        /** When receiving a timing request from the peer port,
            pass it to the ethz_pim_if. */
        bool recvTimingResp(PacketPtr pkt);

        /** When receiving a retry request from the peer port,
            pass it to the ethz_pim_if. */
        void recvRetry();
    };

    /** Slave port of the ethz_pim_if. */
    ethz_PIMIFSlavePort slavePort;

    /** Master port of the ethz_pim_if. */
    ethz_PIMIFMasterPort masterPort;

  public:

    // Initiate a read transaction
    bool sendReadPacket(uint64_t address, uint64_t length, Tick when);
    
    // Initiate a write transaction
    bool sendWritePacket(uint64_t address, uint64_t length, uint8_t* data, Tick when);

    // Added by Erfan: ID of the current master
    MasterID masterId;

    virtual BaseMasterPort& getMasterPort(const std::string& if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort& getSlavePort(const std::string& if_name,
                                        PortID idx = InvalidPortID);

    virtual void init();
    
    typedef ethz_PIMIFParams Params;

    ethz_PIMIF(Params *p);
};

#endif //__ETHZ_PIM_IF_HH__
