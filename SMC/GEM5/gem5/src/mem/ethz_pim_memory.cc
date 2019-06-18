#include "base/random.hh"
#include "mem/ethz_pim_memory.hh"
#include "debug/Drain.hh"
#include "ethz_pim_definitions.h"
#include "ethz_sgraph_definitions.hh"
#include "cpu/thread_context.hh"
#include "mem/noncoherent_xbar.hh"
#include <sstream>
#include "mem/cache/base.hh"
#include "base/stats/text.hh"
#include "sim/pseudo_inst.hh"
#include <fstream>
#include "dev/ethz_dma.hh"
using namespace std;

// This Memory is devoted to the single-processor PIM

//ERFAN
//#define DEBUG_ETHZ_PIM_MEMORY
//#define DEBUG_ETHZ_DMA
//#define DEBUG_COPROCESSOR


#define S0      ((pmemAddr + PIM_SREG_ADDR + 0*(pim_arch/8) - range.start()))       // pointer to PIM_SREG[0]
#define S1      ((pmemAddr + PIM_SREG_ADDR + 1*(pim_arch/8) - range.start()))       // pointer to PIM_SREG[1]
#define S2      ((pmemAddr + PIM_SREG_ADDR + 2*(pim_arch/8) - range.start()))       // pointer to PIM_SREG[2]
#define S3      ((pmemAddr + PIM_SREG_ADDR + 3*(pim_arch/8) - range.start()))       // pointer to PIM_SREG[2]

static bool STANDALONE_SIMULATION;

ethz_PIMMemory::ethz_PIMMemory(const ethz_PIMMemoryParams* p) :
    SimpleMemory(p),
    PIM_ADDRESS_BASE(p->PIM_ADDRESS_BASE),
    PIM_DEBUG_ADDR(p->PIM_DEBUG_ADDR),
    PIM_ERROR_ADDR(p->PIM_ERROR_ADDR),
    PIM_INTR_ADDR(p->PIM_INTR_ADDR),
    PIM_COMMAND_ADDR(p->PIM_COMMAND_ADDR),
    PIM_STATUS_ADDR(p->PIM_STATUS_ADDR),
	PIM_SLICETABLE_ADDR(p->PIM_SLICETABLE_ADDR),
	PIM_SLICECOUNT_ADDR(p->PIM_SLICECOUNT_ADDR),
	PIM_SLICEVSTART_ADDR(p->PIM_SLICEVSTART_ADDR),
	PIM_M5_ADDR(p->PIM_M5_ADDR),
	PIM_M5_D1_ADDR(p->PIM_M5_D1_ADDR),
	PIM_M5_D2_ADDR(p->PIM_M5_D2_ADDR),
    PIM_VREG_ADDR(p->PIM_VREG_ADDR),
    PIM_VREG_SIZE(p->PIM_VREG_SIZE),
    PIM_SREG_ADDR(p->PIM_SREG_ADDR),
    PIM_SREG_COUNT(p->PIM_SREG_COUNT),
    PIM_DTLB_IDEAL_REFILL_ADDR(p->PIM_DTLB_IDEAL_REFILL_ADDR),
    PIM_DMA_MEM_ADDR_ADDR(p->PIM_DMA_MEM_ADDR_ADDR),
    PIM_DMA_SPM_ADDR_ADDR(p->PIM_DMA_SPM_ADDR_ADDR),
    PIM_DMA_NUMBYTES_ADDR(p->PIM_DMA_NUMBYTES_ADDR),
    PIM_DMA_COMMAND_ADDR(p->PIM_DMA_COMMAND_ADDR),
    PIM_DMA_CLI_ADDR(p->PIM_DMA_CLI_ADDR),
    PIM_COPROCESSOR_CMD_ADDR(p->PIM_COPROCESSOR_CMD_ADDR),
	SYSTEM_NUM_CPU(p->SYSTEM_NUM_CPU),
    pim_arch(p->pim_arch),
    HMC_ATOMIC_INCR_ADDR(p->HMC_ATOMIC_INCR_ADDR),
    pim_system(nullptr),
    pim_cpu(nullptr)
{
    _dma_num_bytes = 0;
    _dma_mem_addr = 0;
	prev_time_stamp = 0;
	prev_time_stamp_id = '?';
	if ( PIM_DEBUG_ADDR )
		cout << "PIM Memory: PIM_DEBUG_ADDR: 0x" << hex << PIM_DEBUG_ADDR << dec << endl;

	// cout << "gem5: sizeof(Addr)=" << sizeof(Addr) << endl;
	// cout << "gem5: sizeof(void*)=" << sizeof(void*) << endl;
	// cout << "gem5: sizeof(int)=" << sizeof(int) << endl;
	assert(sizeof(Addr) == sizeof(unsigned long));
	assert(sizeof(Addr) == sizeof(void*));
}

void ethz_PIMMemory::init()
{
	SimpleMemory::init();
	STANDALONE_SIMULATION = false;
	vector<System *> slist = System::systemList;
	for ( auto it = slist.begin(); it != slist.end(); it++ )
	{
		if ( (*it)->name() == "system.pim_sys" )
			pim_system = (*it);
	}
	if ( pim_system == nullptr )
	{
		cout << "Warning: system.pim_sys not found, assuming standalone simulation of the PIM system" << endl;
		STANDALONE_SIMULATION=true;
		pim_system = (*slist.begin());
	}
	
	if ( ! STANDALONE_SIMULATION )
	{
		SimObject* f = SimObject::find("system.pim_sys.dtlb");
		if ( f )
			dtlb = (ethz_TLB*) f;
		else
			panic("Cannot find a pointer to system.pim_sys.dtlb");

        f = SimObject::find("system.pim_sys.dma");
        if ( f )
            dma = (ethz_DMA*) f;
        else
            panic("Cannot find a pointer to system.pim_sys.dma");
	}
	else
	{
		SimObject* f = SimObject::find("system.dtlb");
		if ( f )
			dtlb = (ethz_TLB*) f;
		else
			panic("Cannot find a pointer to system.dtlb");

        f = SimObject::find("system.dma");
        if ( f )
            dma = (ethz_DMA*) f;
        else
            panic("Cannot find a pointer to system.dma");
	}
	
	std::vector<ThreadContext *> &tcvec = pim_system->threadContexts;
	pim_cpu = tcvec[0]->getCpuPtr(); // cpu_id = 0
    assert( pim_cpu );
	//cout << "PIMMemory: CPU Name: " << pim_cpu->name() << endl;

	/*
	Get a pointer to system caches, so that we can flush them
	*/
	// Search for L2 cache
	SimObject* f = SimObject::find("system.l2");
	if ( f )
		system_caches.push_back(f);
	else
		cout << "Warning: system.l2 is not present!" << endl;
	
	for ( int i=0; i<SYSTEM_NUM_CPU; i++ )
	{
		string id="";
		string name="";
		if ( SYSTEM_NUM_CPU != 1)
		{
			ostringstream oss;
			oss << i;
			id = oss.str();
		}
		// Search for dcache
		name = "system.cpu";
		name += id;
		name += ".dcache";

		SimObject* f = SimObject::find(name.c_str());
		if ( f )
			system_caches.push_back(f);
		else
			cout << "Warning: " << name << " is not present!" << endl;
	}

    dma->dtlb = dtlb;   // Pass the pointer of DTLB to DMA
    dma->pim_cpu = pim_cpu;
    dtlb->dma = dma;    // Pass the pointer of DMA to DTLB
    dma->isTimingMode = pim_system->isTimingMode();
    dma->tc = tcvec[0];
    dma->pim_mem = this;
}

Tick
ethz_PIMMemory::recvAtomic(PacketPtr pkt)
{
	// DEBUG
	#ifdef DEBUG_ETHZ_PIM_MEMORY
	cout << "PIMMemory: [Atomic]" << pkt->cmdString() << " " << hex << pkt->getAddr() << dec << " DATA[0]:" << (int)(*pkt->getPtr<uint8_t>())<< " SIZE:" << pkt->getSize() << endl;
	#endif
	if ( pkt ->isWrite() )
	{
		//************************
		// DEBUG THE PIM REGISTERS
		#ifdef DEBUG_ETHZ_PIM_MEMORY		
		if ( pkt-> getAddr() == PIM_STATUS_ADDR || pkt-> getAddr() == PIM_COMMAND_ADDR || pkt-> getAddr() == PIM_INTR_ADDR || pkt-> getAddr() == PIM_ERROR_ADDR )
			cout << "PIMMemory: [Atomic]" << pkt->cmdString() << " " << pkt->getAddr() << " DATA[0]:" << (char)(*pkt->getPtr<uint8_t>())<< " SIZE:" << pkt->getSize() << endl;
		#endif
		//************************

        ////////////////////////////////////////		
		if ( pkt-> getAddr() == PIM_DEBUG_ADDR )
		{
            assert(pkt->getSize() == 1);
			uint8_t* c = pkt->getPtr<uint8_t>();
			char cc = ((char)(*c));
			if ( cc != '\n' && cc != '\r' )
				cout << cc;
			else
			if ( cc == '\n' )
			{
				cout << endl;
				cout.flush();
			}
		}
		else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_ERROR_ADDR )
		{
            assert(pkt->getSize() == 1);
			uint8_t* c = pkt->getPtr<uint8_t>();
			uint8_t code = *c;
			if ( code != PIM_ERROR_NONE )
			{
				cout << " <PIM_ERROR_REG="<< (int)code << "> Error happened in the PIM device, exiting simulator" << endl;
				exit(1);
			}
			else
                cout << " <PIM_ERROR_REG="<< (int)code << "> Ignored" << endl;
		}
		else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_INTR_ADDR )
		{
            assert(pkt->getSize() == 1);
            uint8_t* c = pkt->getPtr<uint8_t>();
            if ( *c == 0 )
                cout << " <PIM_INTR_REG=0> Ignored" << endl;
            else
            {
                pim_cpu->postInterrupt(ArmISA::INT_IRQ, 0);
                pim_cpu->clearInterrupt(ArmISA::INT_IRQ, 0);
            }
		}
		else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_SLICETABLE_ADDR )
		{	
			assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
			{
                uint32_t* c = pkt->getPtr<uint32_t>();
                dtlb->PIM_SLICETABLE_PTR = *c;
                cout << " <PIM_SLICETABLE_PTR=0x" << hex << dtlb->PIM_SLICETABLE_PTR << dec << ">" << endl;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                dtlb->PIM_SLICETABLE_PTR = *c;
                cout << " <PIM_SLICETABLE_PTR=0x" << hex << dtlb->PIM_SLICETABLE_PTR << dec << ">" << endl;
            }
		}
		else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_SLICECOUNT_ADDR )
		{	
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                uint32_t* c = pkt->getPtr<uint32_t>();
                dtlb->PIM_SLICECOUNT = *c;
                cout << " <PIM_SLICECOUNT=0x" << hex << dtlb->PIM_SLICECOUNT << dec << ">" << endl;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                dtlb->PIM_SLICECOUNT = *c;
                cout << " <PIM_SLICECOUNT=0x" << hex << dtlb->PIM_SLICECOUNT << dec << ">" << endl;
            }
		}
		else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_SLICEVSTART_ADDR )
		{	
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                uint32_t* c = pkt->getPtr<uint32_t>();
                dtlb->PIM_SLICEVSTART = *c;
                cout << " <PIM_SLICEVSTART=0x" << hex << dtlb->PIM_SLICEVSTART << dec << ">" << endl;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                dtlb->PIM_SLICEVSTART = *c;
                cout << " <PIM_SLICEVSTART=0x" << hex << dtlb->PIM_SLICEVSTART << dec << ">" << endl;
            }
		}
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DMA_NUMBYTES_ADDR )
        {
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                uint32_t* c = pkt->getPtr<uint32_t>();
                _dma_num_bytes = *c;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                _dma_num_bytes = *c;
            }
            #ifdef DEBUG_ETHZ_DMA
            cout << " <PIM_DMA_NUMBYTES=0x" << hex << (unsigned) _dma_num_bytes << dec << ">" << endl;
            #endif
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DMA_MEM_ADDR_ADDR )
        {
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                uint32_t* c = pkt->getPtr<uint32_t>();
                _dma_mem_addr = *c;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                _dma_mem_addr = *c;
            }
            #ifdef DEBUG_ETHZ_DMA
            cout << " <PIM_DMA_MEM_ADDR=0x" << hex << (unsigned) _dma_mem_addr << dec << ">" << endl;
            #endif
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DMA_SPM_ADDR_ADDR )
        {
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                uint32_t* c = pkt->getPtr<uint32_t>();
                _dma_spm_addr = *c;
            }
            else  // 64b PIM
            {
                uint64_t* c = pkt->getPtr<uint64_t>();
                _dma_spm_addr = *c;
            }
            #ifdef DEBUG_ETHZ_DMA
            cout << " <PIM_DMA_SPM_ADDR=0x" << hex << (unsigned) _dma_spm_addr << dec << ">" << endl;
            #endif
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_COPROCESSOR_CMD_ADDR )
        {
            assert(pkt->getSize() == 1);
            uint8_t* c = pkt->getPtr<uint8_t>();
            if ( pim_arch == 32 ) // 32b PIM
                run_coprocessor_32b(*c);
            else  // 64b PIM
                run_coprocessor_64b(*c);
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DMA_COMMAND_ADDR )
        {
            assert(pkt->getSize() == 1);
            uint8_t* c = pkt->getPtr<uint8_t>();
            uint8_t cmd = (*c);
            if ( cmd == 0 )
                cout << " <PIM_DMA_COMMAND=0>" << endl;
            else
            {
                #ifdef DEBUG_ETHZ_DMA
                cout << " <PIM_DMA_COMMAND=0x" << hex << (unsigned)cmd << " _dma_spm_addr=0x" << _dma_spm_addr << " _dma_mem_addr=0x" << _dma_mem_addr << " _dma_num_bytes=" << dec << _dma_num_bytes << ">" << endl;
                #endif

                assert(_dma_num_bytes != 0 );
                assert(_dma_mem_addr != 0 );
                assert (pim_system->isTimingMode());
                // Translate to physical address (a simple remapping)
                _dma_spm_addr += PIM_ADDRESS_BASE;
                if ( _dma_spm_addr < PIM_VREG_ADDR || _dma_spm_addr + _dma_num_bytes > PIM_VREG_ADDR + PIM_VREG_SIZE )
                {
                    cout << "panic: VREG=[0x" << hex << PIM_VREG_ADDR << ", 0x" << PIM_VREG_ADDR + PIM_VREG_SIZE-1 
                        << "] Access=[0x" << _dma_spm_addr << ", 0x" << _dma_spm_addr + _dma_num_bytes-1 << "]" << dec << endl;
                    panic( "DMA access must be within the PIM's Vector Register File");
                }
                uint8_t *vreg_ptr = pmemAddr + _dma_spm_addr - range.start();
                if ( (cmd & 0b10000000) == PIM_DMA_READ )
                    dma->initiate_read(vreg_ptr, _dma_mem_addr, _dma_num_bytes, (cmd & 0b01111111));
                else
                if ( (cmd & 0b10000000) == PIM_DMA_WRITE )
                    dma->initiate_write(vreg_ptr, _dma_mem_addr, _dma_num_bytes, (cmd & 0b01111111));
                else
                    panic("Illegal DMA command!");
            }
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DMA_CLI_ADDR )
        {
            assert(pkt->getSize() == 1);
            uint8_t* c = pkt->getPtr<uint8_t>();
            uint8_t cmd = (*c);
            if ( cmd == 0 )
                cout << " <PIM_DMA_CLI=0>" << endl;
            else
            {
                #ifdef DEBUG_ETHZ_DMA
                cout << " <PIM_DMA_CLI=0x" << hex << (unsigned)cmd << dec << endl;
                #endif
                dma->wait_on_resource(cmd);
            }
        }
        else
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == HMC_ATOMIC_INCR_ADDR )
        {
            assert(pkt->getSize() == pim_arch/8);
            if ( pim_arch == 32 ) // 32b PIM
            {
                assert( *(pkt->getPtr<uint32_t>()) == 0 );
            }
            else  // 64b PIM
            {
                assert( *(pkt->getPtr<uint64_t>()) == 0 );
            }            
        }
        ////////////////////////////////////////        
        if ( pkt-> getAddr() == PIM_DTLB_IDEAL_REFILL_ADDR )
        {
            panic("PIMMemory should never see an access to this register: PIM_DTLB_IDEAL_REFILL_ADDR");
        }
        else
        ////////////////////////////////////////        
		if ( pkt-> getAddr() == PIM_M5_ADDR )
		{	
            assert(pkt->getSize() == 1);
			/*
			Debugging facility added by Erfan
			Writing values to this register will give commands to gem5
			*/
			uint8_t* c = pkt->getPtr<uint8_t>();
			uint8_t CMD = ((uint8_t)(*c));
			//cout << "<M5 Command Received [" << ((uint8_t)(*c)) << "]>" << endl;
			/*
			Print the current simulation time of gem5 + the number sent by the command
			*/
			if ( CMD >= PIM_TIME_STAMP && CMD <= PIM_TIME_STAMP_MAX)
			{
				cout << dec << curTick() << " <timestamp"<< CMD << "> ######################## <prevstamp"<< prev_time_stamp_id << "> Diff: " << curTick() - prev_time_stamp << " stats.dump() stats.reset()" << endl;

                Stats::Text::time_stamp_id = CMD;
				// Also dump stats:
                //std::vector<ThreadContext *> &tcvec = pim_system->threadContexts;
				//PseudoInst::dumpresetstats(tcvec[0], 0, 0);
                Stats::dump();
                Stats::reset();
				prev_time_stamp = curTick();
				prev_time_stamp_id = CMD;
			}
			else
			switch (CMD)
			{
			/****************************************/
			/*
			Enable packet logging inside the NoncoherentXBar
			*/
			case PIM_ENABLE_PACKET_LOG:
				cout << " <PIM_M5_REG:Enable Packet Logging>" << endl;
				NoncoherentXBar::enable_packet_log = true;
				break;
			/*
			Disable packet logging inside the NoncoherentXBar
			*/
			case PIM_DISABLE_PACKET_LOG:
				cout << " <PIM_M5_REG:Disable Packet Logging>" << endl;
				NoncoherentXBar::enable_packet_log = false;
				break;
			/*
			Exit gem5 (similar to the m5 exit command)
			*/
			case PIM_EXIT_GEM5:
				cout << " <PIM_M5_REG:Exit gem5>" << endl;
				exit(0);
				break;
			/*
			Simulation hack:
			Ask the host processor to flush its caches.
			This operation happens only for the slices and the
			slice table itself.
			*/
			case PIM_HOST_CACHE_FLUSH_HACK:
				// cout << "<Host Cache Flush>" << endl;
				ask_host_to_flush_the_caches();
				break;
            case PIM_M5_IGNORE:
                cout << " <PIM_M5_REG:Nop>" << endl;
                break;
			default:
				panic("Command Not implemented!");
			}

		}
	}
	return SimpleMemory::recvAtomic(pkt);
}

void
ethz_PIMMemory::recvFunctional(PacketPtr pkt)
{
	// DEBUG
	#ifdef DEBUG_ETHZ_PIM_MEMORY
	cout << "PIMMemory: [Functional]" << pkt->cmdString() << " " << hex << pkt->getAddr() << dec << " DATA[0]" << (int)(*pkt->getPtr<uint8_t>())<< " SIZE:" << pkt->getSize() << endl;
	#endif
	SimpleMemory::recvFunctional(pkt);
}

bool
ethz_PIMMemory::recvTimingReq(PacketPtr pkt)
{
	// DEBUG
	#ifdef DEBUG_ETHZ_PIM_MEMORY
	cout << "PIMMemory: [Timing]" << pkt->cmdString() << " " << hex << pkt->getAddr() << dec << " DATA[0]:" << (int)(*pkt->getPtr<uint8_t>())<< " SIZE:" << pkt->getSize() << endl;
	#endif
	return SimpleMemory::recvTimingReq(pkt);
}

void
ethz_PIMMemory::ask_host_to_flush_the_caches()
{
	// cout << "ethz_PIMMemory::ask_host_to_flush_the_caches()" << endl;
	unsigned long start_addr = __read_mem_1word(PIM_M5_D1_ADDR);
	unsigned long end_addr   = __read_mem_1word(PIM_M5_D2_ADDR);
    // cout << " PIM_M5_D1_REG=" << hex << start_addr << dec << endl;
    // cout << " PIM_M5_D2_REG=" << hex << end_addr   << dec << endl;
    for ( int i=0; i< system_caches.size(); i++ )
	{
	    BaseCache* c = dynamic_cast<BaseCache*>(system_caches[i]);
	    //cout << "CACHE: " << c->name() << endl;
	    c->ethz_flush_range(start_addr, end_addr);
	}
}

unsigned long
ethz_PIMMemory::__read_mem_1word(unsigned long int address)
{
    uint8_t *hostAddr = pmemAddr + address - range.start();
    if ( pim_arch == 32 )
    {
        uint32_t *P = (uint32_t*)hostAddr;
        return *P;
    }
    else
    {
        uint64_t *P = (uint64_t*)hostAddr;
        return *P;
    }
}

void
ethz_PIMMemory::__write_mem_1byte(unsigned long int address, uint8_t value)
{
    uint8_t *hostAddr = pmemAddr + address - range.start();
    (*hostAddr) = value;
}

void
ethz_PIMMemory::run_coprocessor_32b(uint8_t cmd)
{
    float f, op0, op1, diff;
    uint32_t u;
    unsigned long addr, rr, count, NODES;
    volatile node_pr_32* nodes_P;

    switch (cmd)
    {
        /************/
        case CMD_NOP:
            cout << " <PIM_COPROCESSOR_CMD_ADDR=Nop>" << endl;
        break;
        /************/
        case CMD_TO_FLOAT:
            u = *((uint32_t*)S0);
            f = (float)u;
            *((float*)S1) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_TO_FLOAT: Int=" << u << " Float=" << f << endl;
            #endif
        break;
        /************/
        case CMD_TO_FIXED:
            f = *((float*)S0);
            f *= 1000000.0;
            u = (uint32_t)f;
            *((uint32_t*)S1) = u;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_TO_FIXED: Float*1000000=" << f << " Fixed=" << u << endl;
            #endif
        break;
        /************/
        case CMD_FDIV:
            op0 = *((float*)S0);
            op1 = *((float*)S1);
            f = op0 / op1;
            *((float*)S2) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FDIV: OP0=" << op0 << " OP1=" << op1 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FMUL:
            op0 = *((float*)S0);
            op1 = *((float*)S1);
            f = op0 * op1;
            *((float*)S2) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FMUL: OP0=" << op0 << " OP1=" << op1 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FCMP:
            op0 = *((float*)S0);
            op1 = *((float*)S1);
            f = (op0 > op1)?1.0:0;
            *((float*)S2) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FCMP: OP0=" << op0 << " OP1=" << op1 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FSUB:
            op0 = *((float*)S0);
            op1 = *((float*)S1);
            f = op0 - op1;
            *((float*)S2) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FSUB: OP0=" << op0 << " OP1=" << op1 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FACC:
            op0 = *((float*)S0);
            op1 = *((float*)S1);
            f = op0 + op1;
            *((float*)S1) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FACC: OP0=" << op0 << " OP1=" << op1 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FABS:
            op0 = *((float*)S0);
            f = (op0 >= 0) ? op0 : -op0;
            *((float*)S1) = f;
            #ifdef DEBUG_COPROCESSOR
            cout << "CMD_FABS: OP=" << op0 << " Res=" << f << endl;
            #endif
        break;
        /************/
        case CMD_FPRINT:
            op0 = *((float*)S0);
            cout << "CMD_FPRINT: { OP=" << op0 << " }" << endl;
        break;
        /************/
        /* Custom series of instructions for PageRank*/
        case CMD_CUSTOM_PR1:
        addr = *(uint32_t*)(S0) + PIM_ADDRESS_BASE;
        count = *(uint32_t*)(S1);
        NODES = *(uint32_t*)(S2);
        nodes_P = (node_pr_32*)(pmemAddr + addr - range.start());

        #ifdef DEBUG_COPROCESSOR
        cout << "ID[0]=" << nodes_P[0].ID << " addr=0x" << hex << addr << " count=0x" << count << " NODES=0x" << NODES << dec << endl;
        #endif
        assert(addr >= PIM_VREG_ADDR);
        assert(addr <  PIM_VREG_ADDR + PIM_VREG_SIZE);

        diff=0;
        for ( rr=0; rr<count; rr++ )
        {
            /* absolute value */
            f = nodes_P[rr].next_rank - nodes_P[rr].page_rank;
            if ( f < 0 )
                f = -f;
            diff += f;
            nodes_P[rr].page_rank = nodes_P[rr].next_rank;
            nodes_P[rr].next_rank = 0.15 / (float)NODES;
        }
        /* return back diff to PIM*/
        *(float*)(S3) = diff;

        #ifdef DEBUG_COPROCESSOR
        cout << "partial diff = " << diff << endl;
        #endif
        break;
        /************/
        default:
            panic("Illegal command! (run_coprocessor_32b)");
    }
}

void
ethz_PIMMemory::run_coprocessor_64b(uint8_t cmd)
{
    switch (cmd)
    {
        case CMD_NOP:
            cout << " <PIM_COPROCESSOR_CMD_ADDR=Nop>" << endl;
        break;
        default:
            panic("Not implemented yet! (run_coprocessor_64b)");
    }
}



ethz_PIMMemory*
ethz_PIMMemoryParams::create()
{
    return new ethz_PIMMemory(this);
}

