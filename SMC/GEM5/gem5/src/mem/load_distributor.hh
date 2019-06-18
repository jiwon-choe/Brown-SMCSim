#ifndef __LOAD_DISTRIBUTOR__
#define __LOAD_DISTRIBUTOR__

#include "mem/noncoherent_xbar.hh"
#include "mem/port.hh"
#include "params/LoadDistributor.hh"

using namespace std;

class LoadDistributor : public NoncoherentXBar
{
public:
	
	LoadDistributor(const LoadDistributorParams *p);
	
	// Receive range change only on one of the ports (because they all have the same range)
	virtual void recvRangeChange(PortID master_port_id);
	
	// Receive a request and distribute it among slave ports
	virtual bool recvTimingReq(PacketPtr pkt, PortID slave_port_id);
private:	
	int N_MASTER_PORTS;
	int ROUND_ROBIN_COUNTER;
	int rotate_counter();
};

#endif //__LOAD_DISTRIBUTOR__
