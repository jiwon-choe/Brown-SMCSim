#include "base/random.hh"
#include "mem/load_distributor.hh"

LoadDistributor::LoadDistributor(const LoadDistributorParams* p) :
    NoncoherentXBar(p),
    N_MASTER_PORTS(p->port_master_connection_count)
{
	assert(p->port_slave_connection_count == 1);
	//cout << "LoadDistributor: Load distributor for HMC serial links" << endl;
	ROUND_ROBIN_COUNTER = 0;
}

LoadDistributor*
LoadDistributorParams::create()
{
    return new LoadDistributor(this);
}

// Since this module is a load distributor, all its master ports have the same range
//  so we should keep only one of the ranges and ignore the others
void LoadDistributor::recvRangeChange(PortID master_port_id)
{
	if ( master_port_id == 0 )
	{
	        gotAllAddrRanges = true;
		BaseXBar::recvRangeChange(master_port_id);
	}
	else
		gotAddrRanges[master_port_id] = true;
}

int LoadDistributor::rotate_counter()
{
	int current_value = ROUND_ROBIN_COUNTER;
	ROUND_ROBIN_COUNTER++;
	if ( ROUND_ROBIN_COUNTER == N_MASTER_PORTS )
		ROUND_ROBIN_COUNTER = 0;
	return current_value;
}

bool LoadDistributor::recvTimingReq(PacketPtr pkt, PortID slave_port_id)
{
    // determine the source port based on the id
    SlavePort *src_port = slavePorts[slave_port_id];

    // we should never see express snoops on a non-coherent crossbar
    //assert(!pkt->isExpressSnoop());
    if ( pkt->isExpressSnoop() )	// TODO Erfan has changed this
    {
	    //cout << "EXPRESS SNOOP RECEIVED IN LoadDistributor " << name() << endl; // Erfan
	    return true;
    }
    
    
    //***************************** ERFAN: INTERLEAVING METHOD
//     long LINK = ( pkt->getAddr() >> 8 ) % 16 / 4; // 4 is serial links
//     PortID master_port_id = LINK;
    //***************************** ERFAN: ROUND-ROBIN METHOD
    PortID master_port_id = rotate_counter();
    //********************************************************
    
    

    // test if the layer should be considered occupied for the current
    // port
    if (!reqLayers[master_port_id]->tryTiming(src_port)) {
        //DPRINTF(LoadDistributor, "recvTimingReq: src %s %s 0x%x BUSY\n",
        //        src_port->name(), pkt->cmdString(), pkt->getAddr());
        return false;
    }

    //DPRINTF(LoadDistributor, "recvTimingReq: src %s %s 0x%x\n",
    //        src_port->name(), pkt->cmdString(), pkt->getAddr());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // set the source port for routing of the response
    pkt->setSrc(slave_port_id);

    calcPacketTiming(pkt);
    Tick packetFinishTime = pkt->lastWordDelay + curTick();

    // since it is a normal request, attempt to send the packet
    bool success = masterPorts[master_port_id]->sendTimingReq(pkt);

    if (!success)  {
        // inhibited packets should never be forced to retry
        assert(!pkt->memInhibitAsserted());

        //DPRINTF(LoadDistributor, "recvTimingReq: src %s %s 0x%x RETRY\n",
        //        src_port->name(), pkt->cmdString(), pkt->getAddr());

        // undo the calculation so we can check for 0 again
        pkt->firstWordDelay = pkt->lastWordDelay = 0;

        // occupy until the header is sent
        reqLayers[master_port_id]->failedTiming(src_port,
                                                clockEdge(headerCycles));

        return false;
    }

    reqLayers[master_port_id]->succeededTiming(packetFinishTime);

    // stats updates
    pktCount[slave_port_id][master_port_id]++;
    pktSize[slave_port_id][master_port_id] += pkt_size;
    transDist[pkt_cmd]++;

    return true;
}
