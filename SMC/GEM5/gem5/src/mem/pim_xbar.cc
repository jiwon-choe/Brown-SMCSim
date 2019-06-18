/*
 * Copyright (c) 2011-2014 ARM Limited
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
 * Copyright (c) 2006 The Regents of The University of Michigan
 * All rights reserved.
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
 * Authors: Ali Saidi
 *          Andreas Hansson
 *          William Wang
 */

/**
 * @file
 * Definition of a non-coherent crossbar object.
 */

#include "base/misc.hh"
#include "base/trace.hh"
#include "debug/PIMXBar.hh"
#include "debug/XBar.hh"
#include "mem/pim_xbar.hh"

#include <iostream>
using namespace std;

#define WAIT_ps 1   // Note: this does not have any notion of time, and should not be too large
#define DEBUG_PERIOD_ps 1000000

// Info: This can be enabled/disabled from the software layer using (M5_DISABLE_PACKET_LOG)
//#define ENABLE_PACKET_LOG
#define PACKET_LOG_START    0x80000000
#define PACKET_LOG_END      0x9FFFFFFF
bool PIMXBar::enable_packet_log = false;

PIMXBar::PIMXBar(const PIMXBarParams *p)
    : BaseXBar(p),
    ideal_instr_fetch(p->ideal_instr_fetch),
    ideal_spm_access(p->ideal_spm_access),
    respondEvent(this),
    debugEvent(this)
{
    response_scheduled = false;
    spm_master_port = -1;
    // create the ports based on the size of the master and slave
    // vector ports, and the presence of the default port, the ports
    // are enumerated starting from zero
    for (int i = 0; i < p->port_master_connection_count; ++i) {
        std::string portName = csprintf("%s.master[%d]", name(), i);
        MasterPort* bp = new PIMXBarMasterPort(portName, *this, i);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this,
                                         csprintf(".reqLayer%d", i)));
    }

    // see if we have a default slave device connected and if so add
    // our corresponding master port
    if (p->port_default_connection_count) {
        defaultPortID = masterPorts.size();
        std::string portName = name() + ".default";
        MasterPort* bp = new PIMXBarMasterPort(portName, *this,
                                                      defaultPortID);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this, csprintf(".reqLayer%d",
                                                              defaultPortID)));
    }

    // create the slave ports, once again starting at zero
    for (int i = 0; i < p->port_slave_connection_count; ++i) {
        std::string portName = csprintf("%s.slave[%d]", name(), i);
        SlavePort* bp = new PIMXBarSlavePort(portName, *this, i);
        slavePorts.push_back(bp);
        respLayers.push_back(new RespLayer(*bp, *this,
                                           csprintf(".respLayer%d", i)));
    }

    clearPortCache();
}

PIMXBar::~PIMXBar()
{
    for (auto l: reqLayers)
        delete l;
    for (auto l: respLayers)
        delete l;
}

void
PIMXBar::init()
{
    BaseXBar::init();

    /* Erfan: Find the SPM port to access it ideally */
    if ( ideal_spm_access )
    {
        for (int i = 0; i < masterPorts.size(); i++)
            if ( masterPorts[i]->getSlavePort().name() == "system.Smon-slave" ||
                 masterPorts[i]->getSlavePort().name() == "system.shared_spm.port" )
                {
                    assert(spm_master_port == -1);
                    cout <<  name() << " SPM Port Number = " << i << " [" << masterPorts[i]->name() 
                         << " --> " << masterPorts[i]->getSlavePort().name() << endl;
                    spm_master_port = i;
                }
        assert(spm_master_port != -1);
    }

    //schedule(debugEvent, curTick() + DEBUG_PERIOD_ps);
}

bool
PIMXBar::recvTimingReq(PacketPtr pkt, PortID slave_port_id)
{
    if ( ideal_instr_fetch && pkt->req->getFlags().isSet(Request::INST_FETCH))
    {
        assert(pkt->isRead());
        recvAtomic(pkt, slave_port_id);
        port_list.push(slave_port_id);
        pkt_list.push(pkt);
        if ( !response_scheduled )
        {
            response_scheduled = true;
            schedule(respondEvent, curTick() + WAIT_ps); // 1ps after
        }
        DPRINTF(PIMXBar, "recvTimingReq: from %s %s 0x%x (Ideal Instruction Fetch)\n",
            slavePorts[slave_port_id]->name(), pkt->cmdString(), pkt->getAddr());
        return true;
    }

    // determine the source port based on the id
    SlavePort *src_port = slavePorts[slave_port_id];

    #ifdef ENABLE_PACKET_LOG
    if ( enable_packet_log && pkt->getAddr() >= PACKET_LOG_START && pkt->getAddr() <= PACKET_LOG_END )
    {
        cout << "recvTimingReq " << src_port->name() << " " <<  pkt->cmdString() << " 0x" << hex << pkt->getAddr() << " SIZE:" << dec << pkt->getSize() << "(B) @" << curTick() << endl;// " {";
        // int c=(pkt->getSize()>32)?32:pkt->getSize();
        // for ( int i=0; i<c; i++ )
        //     cout << hex << (unsigned)(pkt->getPtr<uint8_t>()[i]) << " ";
        // cout << "}" << endl;
    }
    #endif

    // we should never see express snoops on a non-coherent crossbar
    //assert(!pkt->isExpressSnoop());
    if ( pkt->isExpressSnoop() )	// TODO Erfan has changed this
    {
	    cout << "EXPRESS SNOOP RECEIVED IN NONCOHERENT_XBAR " << name() << endl;
	    return true;
    }

    // determine the destination based on the address
    PortID master_port_id = findPort(pkt->getAddr());

    if (ideal_spm_access && master_port_id == spm_master_port )
    {
        DPRINTF(PIMXBar, "recvTimingReq: from %s %s 0x%x (Ideal SPM Access)\n", slavePorts[slave_port_id]->name(), pkt->cmdString(), pkt->getAddr());

        /* Access the SPM ideally */
        port_list.push(slave_port_id);
        pkt_list.push(pkt);
        recvAtomic(pkt, slave_port_id);
        if ( !response_scheduled )
        {
            response_scheduled = true;
            schedule(respondEvent, curTick() + WAIT_ps);
        }
        return true;
    }
    
    // test if the layer should be considered occupied for the current
    // port
    if (!reqLayers[master_port_id]->tryTiming(src_port)) {
        DPRINTF(PIMXBar, "recvTimingReq: src %s %s 0x%x BUSY\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());
        return false;
    }

    DPRINTF(PIMXBar, "recvTimingReq: src %s %s 0x%x\n",
            src_port->name(), pkt->cmdString(), pkt->getAddr());

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

        DPRINTF(PIMXBar, "recvTimingReq: src %s %s 0x%x RETRY\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());

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

bool
PIMXBar::recvTimingResp(PacketPtr pkt, PortID master_port_id)
{
    // determine the source port based on the id
    MasterPort *src_port = masterPorts[master_port_id];

    #ifdef ENABLE_PACKET_LOG
    if ( enable_packet_log && pkt->getAddr() >= PACKET_LOG_START && pkt->getAddr() <= PACKET_LOG_END )
    {
        cout << "recvTimingResp " << src_port->name() << " " <<  pkt->cmdString() << " 0x" << hex << pkt->getAddr() << " SIZE:" << dec << pkt->getSize() << "(B) @" << curTick() << endl;// " {";
        int c=(pkt->getSize()>32)?32:pkt->getSize();
        for ( int i=0; i<c; i++ )
            cout << hex << (unsigned)(pkt->getPtr<uint8_t>()[i]) << dec << " ";
        cout << "}" << endl;
    }
    #endif    

    // determine the destination based on what is stored in the packet
    PortID slave_port_id = pkt->getDest();

    // test if the layer should be considered occupied for the current
    // port
    if (!respLayers[slave_port_id]->tryTiming(src_port)) {
        DPRINTF(PIMXBar, "recvTimingResp: src %s %s 0x%x BUSY\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());
        return false;
    }

    DPRINTF(PIMXBar, "recvTimingResp: src %s %s 0x%x\n",
            src_port->name(), pkt->cmdString(), pkt->getAddr());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    calcPacketTiming(pkt);
    Tick packetFinishTime = pkt->lastWordDelay + curTick();

    // send the packet through the destination slave port
    bool success M5_VAR_USED = slavePorts[slave_port_id]->sendTimingResp(pkt);

    // currently it is illegal to block responses... can lead to
    // deadlock
    assert(success);

    respLayers[slave_port_id]->succeededTiming(packetFinishTime);

    // stats updates
    pktCount[slave_port_id][master_port_id]++;
    pktSize[slave_port_id][master_port_id] += pkt_size;
    transDist[pkt_cmd]++;

    return true;
}

void
PIMXBar::recvRetry(PortID master_port_id)
{
    // responses never block on forwarding them, so the retry will
    // always be coming from a port to which we tried to forward a
    // request
    reqLayers[master_port_id]->recvRetry();
}

Tick
PIMXBar::recvAtomic(PacketPtr pkt, PortID slave_port_id)
{
    #ifdef ENABLE_PACKET_LOG
    if ( enable_packet_log && pkt->getAddr() >= PACKET_LOG_START && pkt->getAddr() <= PACKET_LOG_END )
    {
        cout << "recvAtomic " << slavePorts[slave_port_id]->name() << " " <<  pkt->cmdString() << " 0x" << hex << pkt->getAddr() << " SIZE:" << dec << pkt->getSize() << "(B) @" << curTick() << endl;// " {";
        if ( pkt->isWrite())
        {
            int c=(pkt->getSize()>32)?32:pkt->getSize();
            cout << "PKT DATA: ";
            for ( int i=0; i<c; i++ )
                cout << hex << (unsigned)(pkt->getPtr<uint8_t>()[i]) << dec << " ";
            cout << "}" << endl;
        }
    }
    #endif

    DPRINTF(PIMXBar, "recvAtomic: packet src %s addr 0x%x cmd %s\n",
            slavePorts[slave_port_id]->name(), pkt->getAddr(),
            pkt->cmdString());

    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // determine the destination port
    PortID master_port_id = findPort(pkt->getAddr());

    // stats updates for the request
    pktCount[slave_port_id][master_port_id]++;
    pktSize[slave_port_id][master_port_id] += pkt_size;
    transDist[pkt_cmd]++;

    // forward the request to the appropriate destination
    Tick response_latency = masterPorts[master_port_id]->sendAtomic(pkt);

    // add the response data
    if (pkt->isResponse()) {

        #ifdef ENABLE_PACKET_LOG
        if ( enable_packet_log && pkt->getAddr() >= PACKET_LOG_START && pkt->getAddr() <= PACKET_LOG_END )
        {
            int c=(pkt->getSize()>32)?32:pkt->getSize();
            cout << "PKT DATA: ";
            for ( int i=0; i<c; i++ )
                cout << hex << (unsigned)(pkt->getPtr<uint8_t>()[i]) << dec << " ";
            cout << "}" << endl;
        }
        #endif

        pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        pkt_cmd = pkt->cmdToIndex();

        // stats updates
        pktCount[slave_port_id][master_port_id]++;
        pktSize[slave_port_id][master_port_id] += pkt_size;
        transDist[pkt_cmd]++;
    }

    // @todo: Not setting first-word time
    pkt->lastWordDelay = response_latency;
    return response_latency;
}

void
PIMXBar::recvFunctional(PacketPtr pkt, PortID slave_port_id)
{
    #ifdef ENABLE_PACKET_LOG
    if ( enable_packet_log && pkt->getAddr() >= PACKET_LOG_START && pkt->getAddr() <= PACKET_LOG_END )
    {
        cout << "recvFunctional " << slavePorts[slave_port_id]->name() << " " <<  pkt->cmdString() << " 0x" << hex << pkt->getAddr() << " SIZE:" << dec << pkt->getSize() << "(B) @" << curTick() << endl; //" {";
        // int c=(pkt->getSize()>32)?32:pkt->getSize();
        // for ( int i=0; i<c; i++ )
        //     cout << hex << (unsigned)(pkt->getPtr<uint8_t>()[i]) << " ";
        // cout << "}" << endl;
    }
    #endif


    if (!pkt->isPrint()) {
        // don't do DPRINTFs on PrintReq as it clutters up the output
        DPRINTF(PIMXBar,
                "recvFunctional: packet src %s addr 0x%x cmd %s\n",
                slavePorts[slave_port_id]->name(), pkt->getAddr(),
                pkt->cmdString());
    }

    // determine the destination port
    PortID dest_id = findPort(pkt->getAddr());

    // forward the request to the appropriate destination
    masterPorts[dest_id]->sendFunctional(pkt);
}

unsigned int
PIMXBar::drain(DrainManager *dm)
{
    // sum up the individual layers
    unsigned int total = 0;
    for (auto l: reqLayers)
        total += l->drain(dm);
    for (auto l: respLayers)
        total += l->drain(dm);
    return total;
}

PIMXBar*
PIMXBarParams::create()
{
    return new PIMXBar(this);
}

void
PIMXBar::regStats()
{
    // register the stats of the base class and our layers
    BaseXBar::regStats();
    for (auto l: reqLayers)
        l->regStats();
    for (auto l: respLayers)
        l->regStats();
}

// Erfan:
void
PIMXBar::respond()
{
    assert ( pkt_list.size() );
    assert(response_scheduled);

    unsigned  port = port_list.front();
    PacketPtr pkt = pkt_list.front();

    bool success M5_VAR_USED = slavePorts[port]->sendTimingResp(pkt);

    /* Important, if this XBAR operates faster than the CPU and returns data
       before the next clock edge of the CPU has arrived, success will become
       false, in that case we should retry sending later */

    // it is illegal to block responses... can lead to deadlock
    unsigned delay = 0;
    if ( success )
    {
        port_list.pop();
        pkt_list.pop();
    }
    else
    {
        //cout << "respond() waiting for port " << port << endl;
        delay = 1; // 1ps
    }

    if ( !pkt_list.empty() )
    {
        schedule(respondEvent, curTick()+delay); // This should be done very fast
        response_scheduled = true;
    }
    else
        response_scheduled = false;
}

void
PIMXBar::debug()
{
    // cout << "DEBUG pkt_list.size" << pkt_list.size() << " response_scheduled=" << response_scheduled << endl;
    // schedule(debugEvent, curTick() + DEBUG_PERIOD_ps);
}