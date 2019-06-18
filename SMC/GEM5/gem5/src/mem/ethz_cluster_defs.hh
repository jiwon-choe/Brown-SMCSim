#ifndef __MEM_CLUSTER_DEFS_HH__
#define __MEM_CLUSTER_DEFS_HH__

#define STATUS_PE_DISCONNECTED '-'
#define STATUS_PE_UNINIT       'u'
#define STATUS_PE_RUNNING      'r'
#define STATUS_PE_FINISHED     'f'
#define STATUS_SLAVES_FINISHED '1'
#define STATUS_SOME_SLAVES_FINISHED '2'
#define STATUS_SLAVES_RUNNING '3'
#define STATUS_SLAVES_UNINIT '4'

#define MAX_NUM_SEMAPHORES 8
#define MAX_NUM_MASTERS 1800  // Note: each PE can have multiple master ports
#define MAX_NUM_CPUS 257
#define MAX_DMA_RESOURCES_PER_PE 64

#define DMA_CMD_READ  0b01000000
#define DMA_CMD_WRITE 0b11000000
#define DMA_CMD_WAIT  0b10000000

#define DMA_RES_MASK  0b00111111
#define DMA_CMD_MASK  0b11000000

class DMACommand
{
public:
    bool valid;
    unsigned resource;
    unsigned initiator;
    unsigned transaction_id;
    uint32_t SPMADDR; // Address
    uint8_t* SPMPTR; // Actual Pointer 
    uint32_t MEMADDR;
    uint32_t BYTES;
    bool isRead()    { return (CMD == DMA_CMD_READ); }
    bool isWrite()   { return (CMD == DMA_CMD_WRITE); }
    bool isWait()    { return (CMD == DMA_CMD_WAIT); }

    void setCommand( uint8_t c )
    {
        CMD = (c & DMA_CMD_MASK);
        resource = (c & DMA_RES_MASK);
        assert( !( isRead() && isWrite() ));
    }

    void copyFrom( DMACommand& c)
    {
        assert(c.valid);
        valid = c.valid;
        resource = c.resource;
        initiator = c.initiator;
        SPMADDR = c.SPMADDR;
        SPMPTR = c.SPMPTR;
        MEMADDR = c.MEMADDR;
        BYTES = c.BYTES;
        CMD = c.CMD;
        transaction_id = c.transaction_id;
    }

    void print( string msg )
    {
        cout << "[DMA" << resource << "/PE" << initiator << " "
             << (isRead()?"READ":isWrite()?"WRITE":isWait()?"WAIT":"XXX") << hex
             << " SPMADDR=0x" << SPMADDR << " SPMPTR=0x" << (unsigned long) SPMPTR 
             << " MEMADDR=0x" << MEMADDR << dec << " BYTES=" << BYTES << "] TID=" << transaction_id << " " << msg << endl;
    }

private:
    uint8_t CMD;
};

#endif //__MEM_CLUSTER_DEFS_HH__
