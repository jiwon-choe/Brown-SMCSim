#include "base/trace.hh"
#include "debug/ethz_PIMIF.hh"
#include "mem/ethz_pim_if.hh"
#include "params/ethz_PIMIF.hh"

ethz_PIMIF::ethz_PIMIFSlavePort::ethz_PIMIFSlavePort(const std::string& _name,
                                         ethz_PIMIF& _ethz_pim_if,
                                         ethz_PIMIFMasterPort& _masterPort,
                                         Cycles _delay, int _resp_limit,
                                         std::vector<AddrRange> _ranges)
    : SlavePort(_name, &_ethz_pim_if), ethz_pim_if(_ethz_pim_if), masterPort(_masterPort),
      delay(_delay), ranges(_ranges.begin(), _ranges.end()),
      outstandingResponses(0), retryReq(false),
      respQueueLimit(_resp_limit), sendEvent(*this)
{
}

ethz_PIMIF::ethz_PIMIFMasterPort::ethz_PIMIFMasterPort(const std::string& _name,
                                           ethz_PIMIF& _ethz_pim_if,
                                           ethz_PIMIFSlavePort& _slavePort,
                                           Cycles _delay, int _req_limit)
    : MasterPort(_name, &_ethz_pim_if), ethz_pim_if(_ethz_pim_if), slavePort(_slavePort),
      delay(_delay), reqQueueLimit(_req_limit), sendEvent(*this)
{
}

//ofstream ethz_PIMIF::ethz_pim_ifparamsfile("m5out/system.ethz_pim_ifs.params.txt");

ethz_PIMIF::ethz_PIMIF(Params *p)
    : MemObject(p),
      slavePort(p->name + ".slave", *this, masterPort, ticksToCycles(p->delay), p->resp_size, p->ranges),
      masterPort(p->name + ".master", *this, slavePort, ticksToCycles(p->delay), p->req_size),
      masterId(p->system->getMasterId(name()))
{
	//ethz_pim_ifparamsfile << name() << "\tdelay:" << p->delay << "\treq_size" << p->req_size << "\tresp_size" << p->resp_size << endl;
}

BaseMasterPort&
ethz_PIMIF::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "master")
        return masterPort;
    else
        // pass it along to our super class
        return MemObject::getMasterPort(if_name, idx);
}

BaseSlavePort&
ethz_PIMIF::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "slave")
        return slavePort;
    else
        // pass it along to our super class
        return MemObject::getSlavePort(if_name, idx);
}

void
ethz_PIMIF::init()
{
    // make sure both sides are connected and have the same block size
    if (!slavePort.isConnected() || !masterPort.isConnected())
        fatal("Both ports of a ethz_pim_if must be connected.\n");

    // notify the master side  of our address ranges
    slavePort.sendRangeChange();
}

bool
ethz_PIMIF::ethz_PIMIFSlavePort::respQueueFull() const
{
    return outstandingResponses == respQueueLimit;
}

bool
ethz_PIMIF::ethz_PIMIFMasterPort::reqQueueFull() const
{
    return transmitList.size() == reqQueueLimit;
}

bool
ethz_PIMIF::ethz_PIMIFMasterPort::recvTimingResp(PacketPtr pkt)
{
    cout << "PIM: recvTimingResp" << pkt->cmdString() << " " <<  pkt->getAddr() << endl;
	
	
//     // all checks are done when the request is accepted on the slave
//     // side, so we are guaranteed to have space for the response
//     DPRINTF(ethz_PIMIF, "recvTimingResp: %s addr 0x%x\n",
//             pkt->cmdString(), pkt->getAddr());
// 
//     DPRINTF(ethz_PIMIF, "Request queue size: %d\n", transmitList.size());
// 
//     // @todo: We need to pay for this and not just zero it out
//     pkt->firstWordDelay = pkt->lastWordDelay = 0;
// 
//     slavePort.schedTimingResp(pkt, ethz_pim_if.clockEdge(delay));

    return true;
}

bool
ethz_PIMIF::ethz_PIMIFSlavePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(ethz_PIMIF, "recvTimingReq: %s addr 0x%x\n",
            pkt->cmdString(), pkt->getAddr());
    
    cout << "PIM: recvTimingReq" << pkt->cmdString() << " " <<  pkt->getAddr() << endl;
    
    return true;

//     // we should not see a timing request if we are already in a retry
//     assert(!retryReq);
// 
//     DPRINTF(ethz_PIMIF, "Response queue size: %d outresp: %d\n",
//             transmitList.size(), outstandingResponses);
// 
//     // if the request queue is full then there is no hope
//     if (masterPort.reqQueueFull()) {
//         DPRINTF(ethz_PIMIF, "Request queue full\n");
//         retryReq = true;
//     } else {
//         // look at the response queue if we expect to see a response
//         bool expects_response = pkt->needsResponse() &&
//             !pkt->memInhibitAsserted();
//         if (expects_response) {
//             if (respQueueFull()) {
//                 DPRINTF(ethz_PIMIF, "Response queue full\n");
//                 retryReq = true;
//             } else {
//                 // ok to send the request with space for the response
//                 DPRINTF(ethz_PIMIF, "Reserving space for response\n");
//                 assert(outstandingResponses != respQueueLimit);
//                 ++outstandingResponses;
// 
//                 // no need to set retryReq to false as this is already the
//                 // case
//             }
//         }
// 
//         if (!retryReq) {
//             // @todo: We need to pay for this and not just zero it out
//             pkt->firstWordDelay = pkt->lastWordDelay = 0;
// 
//             masterPort.schedTimingReq(pkt, ethz_pim_if.clockEdge(delay));
//         }
//     }
// 
//     // remember that we are now stalling a packet and that we have to
//     // tell the sending master to retry once space becomes available,
//     // we make no distinction whether the stalling is due to the
//     // request queue or response queue being full
//     return !retryReq;
}

void
ethz_PIMIF::ethz_PIMIFSlavePort::retryStalledReq()
{
    if (retryReq) {
        DPRINTF(ethz_PIMIF, "Request waiting for retry, now retrying\n");
        retryReq = false;
        sendRetry();
    }
}

bool
ethz_PIMIF::ethz_PIMIFMasterPort::schedTimingReq(PacketPtr pkt, Tick when)
{
    // If we expect to see a response, we need to restore the source
    // and destination field that is potentially changed by a second
    // crossbar
    if (!pkt->memInhibitAsserted() && pkt->needsResponse()) {
        // Update the sender state so we can deal with the response
        // appropriately
        pkt->pushSenderState(new RequestState(pkt->getSrc()));
    }

    // If we're about to put this packet at the head of the queue, we
    // need to schedule an event to do the transmit.  Otherwise there
    // should already be an event scheduled for sending the head
    // packet.
    if (transmitList.empty()) {
        ethz_pim_if.schedule(sendEvent, when);
    }
    
    //Erfan: to avoid buffer overflow
    if (transmitList.size() == reqQueueLimit)
	    return false;

    assert(transmitList.size() != reqQueueLimit);
    transmitList.push_back(DeferredPacket(pkt, when));
    
    return true;
}

void
ethz_PIMIF::ethz_PIMIFSlavePort::schedTimingResp(PacketPtr pkt, Tick when)
{
    // This is a response for a request we forwarded earlier.  The
    // corresponding request state should be stored in the packet's
    // senderState field.
    RequestState *req_state =
        dynamic_cast<RequestState*>(pkt->popSenderState());
    assert(req_state != NULL);
    pkt->setDest(req_state->origSrc);
    delete req_state;

    // the ethz_pim_if assumes that at least one crossbar has set the
    // destination field of the packet
    assert(pkt->isDestValid());
    DPRINTF(ethz_PIMIF, "response, new dest %d\n", pkt->getDest());

    // If we're about to put this packet at the head of the queue, we
    // need to schedule an event to do the transmit.  Otherwise there
    // should already be an event scheduled for sending the head
    // packet.
    if (transmitList.empty()) {
        ethz_pim_if.schedule(sendEvent, when);
    }

    transmitList.push_back(DeferredPacket(pkt, when));
}

void
ethz_PIMIF::ethz_PIMIFMasterPort::trySendTiming()
{
    assert(!transmitList.empty());

    DeferredPacket req = transmitList.front();

    assert(req.tick <= curTick());

    PacketPtr pkt = req.pkt;

    DPRINTF(ethz_PIMIF, "trySend request addr 0x%x, queue size %d\n",
            pkt->getAddr(), transmitList.size());

    if (sendTimingReq(pkt)) {
        // send successful
        transmitList.pop_front();
        DPRINTF(ethz_PIMIF, "trySend request successful\n");

        // If there are more packets to send, schedule event to try again.
        if (!transmitList.empty()) {
            DeferredPacket next_req = transmitList.front();
            DPRINTF(ethz_PIMIF, "Scheduling next send\n");
            ethz_pim_if.schedule(sendEvent, std::max(next_req.tick,
                                                ethz_pim_if.clockEdge()));
        }

        // if we have stalled a request due to a full request queue,
        // then send a retry at this point, also note that if the
        // request we stalled was waiting for the response queue
        // rather than the request queue we might stall it again
        slavePort.retryStalledReq();
    }

    // if the send failed, then we try again once we receive a retry,
    // and therefore there is no need to take any action
}

void
ethz_PIMIF::ethz_PIMIFSlavePort::trySendTiming()
{
    assert(!transmitList.empty());

    DeferredPacket resp = transmitList.front();

    assert(resp.tick <= curTick());

    PacketPtr pkt = resp.pkt;

    DPRINTF(ethz_PIMIF, "trySend response addr 0x%x, outstanding %d\n",
            pkt->getAddr(), outstandingResponses);

    if (sendTimingResp(pkt)) {
        // send successful
        transmitList.pop_front();
        DPRINTF(ethz_PIMIF, "trySend response successful\n");

        assert(outstandingResponses != 0);
        --outstandingResponses;

        // If there are more packets to send, schedule event to try again.
        if (!transmitList.empty()) {
            DeferredPacket next_resp = transmitList.front();
            DPRINTF(ethz_PIMIF, "Scheduling next send\n");
            ethz_pim_if.schedule(sendEvent, std::max(next_resp.tick,
                                                ethz_pim_if.clockEdge()));
        }

        // if there is space in the request queue and we were stalling
        // a request, it will definitely be possible to accept it now
        // since there is guaranteed space in the response queue
        if (!masterPort.reqQueueFull() && retryReq) {
            DPRINTF(ethz_PIMIF, "Request waiting for retry, now retrying\n");
            retryReq = false;
            sendRetry();
        }
    }

    // if the send failed, then we try again once we receive a retry,
    // and therefore there is no need to take any action
}

void
ethz_PIMIF::ethz_PIMIFMasterPort::recvRetry()
{
    trySendTiming();
}

void
ethz_PIMIF::ethz_PIMIFSlavePort::recvRetry()
{
    trySendTiming();
}

bool DELETE_ME=false;

Tick
ethz_PIMIF::ethz_PIMIFSlavePort::recvAtomic(PacketPtr pkt)
{
	cout << "PIM recvAtomic" << pkt->cmdString() << " " << pkt->getAddr() << endl;

if ( !DELETE_ME)
{
    // SENDING A PACKET TO MEMORY:
    char data[4] = {'a', 'b', 'c', 'd'};
    ethz_pim_if.sendWritePacket(0x80011100, 4, (uint8_t*)data, curTick()+10000);
    ethz_pim_if.sendReadPacket(0x80011100, 4, curTick()+100000);
DELETE_ME = true;
	
}

	return 0; //TODO ERFAN
//     return delay * ethz_pim_if.clockPeriod() + masterPort.sendAtomic(pkt);


}

void
ethz_PIMIF::ethz_PIMIFSlavePort::recvFunctional(PacketPtr pkt)
{
	cout << "PIM recvFunctional" << pkt->cmdString() << " " << pkt->getAddr() << endl;
	
//     pkt->pushLabel(name());
// 
//     // check the response queue
//     for (auto i = transmitList.begin();  i != transmitList.end(); ++i) {
//         if (pkt->checkFunctional((*i).pkt)) {
//             pkt->makeResponse();
//             return;
//         }
//     }
// 
//     // also check the master port's request queue
//     if (masterPort.checkFunctional(pkt)) {
//         return;
//     }
// 
//     pkt->popLabel();
// 
//     // fall through if pkt still not satisfied
//     masterPort.sendFunctional(pkt);
}

bool
ethz_PIMIF::ethz_PIMIFMasterPort::checkFunctional(PacketPtr pkt)
{
    bool found = false;
    auto i = transmitList.begin();

    while(i != transmitList.end() && !found) {
        if (pkt->checkFunctional((*i).pkt)) {
            pkt->makeResponse();
            found = true;
        }
        ++i;
    }

    return found;
}

AddrRangeList
ethz_PIMIF::ethz_PIMIFSlavePort::getAddrRanges() const
{
	return ranges;
}

ethz_PIMIF *
ethz_PIMIFParams::create()
{
    return new ethz_PIMIF(this);
}

bool ethz_PIMIF::sendReadPacket(uint64_t address, uint64_t length,   Tick when)
{
	///cout << "Erfan: (ethz_PIMIF.cc) Scheduling a send packet ... " << endl;
	bool successful = false;
	Request::Flags flags = 0;
	Request *req = new Request(address, length, flags, masterId);
	PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
	pkt->setSrc(0);
// 	SequenceNumber* s = new SequenceNumber();
// 	s->seqNum = seqNum;
// 	pkt->pushSenderState(s);
	uint8_t* empty = new uint8_t[length];
	*empty = 0;
	pkt->dataDynamicArray(empty);
	successful = masterPort.schedTimingReq(pkt, when);
	cout << "Erfan: (ethz_PIMIF.cc) READ packet was scheduled for sending [Success=" << successful << "]" << endl;
	if (!successful)
	{
		delete pkt->senderState;
		delete pkt->req;
		pkt->deleteData();
		delete pkt;
	}
	return successful;
}

bool ethz_PIMIF::sendWritePacket(uint64_t address, uint64_t length, uint8_t* data, Tick when)
{
	///cout << "Erfan: (ethz_PIMIF.cc) Scheduling a send packet ... " << endl;
	bool successful = false;
	Request::Flags flags = 0;
	Request *req = new Request(address, length, flags, masterId);
	PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
	uint8_t* empty = new uint8_t[length];
	pkt->dataDynamicArray(empty);
	pkt->setData(data);
	pkt->setSrc(0);
// 	SequenceNumber* s = new SequenceNumber();
// 	s->seqNum = seqNum;
// 	pkt->pushSenderState(s);
	successful = masterPort.schedTimingReq(pkt, when);
	cout << "Erfan: (ethz_PIMIF.cc) WRITE packet was scheduled for sending [Success=" << successful << "]" << endl;
	if (!successful)
	{
		delete pkt->senderState;
		delete pkt->req;
		pkt->deleteData();
		delete pkt;
	}
	return successful;
}

