#ifndef _PIM_API_H_
#define _PIM_API_H_

// Headers
#include <stdint.h>
#include <sys/mman.h>	// mmap
#include <sys/ioctl.h>  // ioctl
#include <fcntl.h>
#include <stdlib.h>		// atoi, strtol
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <exception>
#include "pim_task.hh"	// PIMTask, PIMData
#include "pim_utils.hh"
#include <iostream>

/*****************************************************/
// PIM API class
// Notice: this class can be linked as a static library to the user level application
class PIMAPI
{
public:
	PIMAPI(int id=0);							// Open the PIM device with specified PIM ID and allocate it in memory
	~PIMAPI();							// Close the PIM device with specified PIM ID and deallocate the memory

	/* Offload methods */
	void offload_kernel(char* name);	// Offload a kernel.hex code to the device
	void offload_task(PIMTask* task);	// Offload a task to the PIM device (but not execute it yet)

	/* Start/stop the computation task */
	void start_computation( uint8_t command );	// Wake the PIM up and give command to execute	[NON-BLOCKING]
	void wait_for_completion_simple();					// Wait until PIM finishes execution (JIWON: does not sleep, and does not send NOP, since PIM kernel does it) 			[BLOCKING]
	void wait_for_completion();					// Wait until PIM finishes executino 			[BLOCKING]

	/* Commands to give to the PIM device (start_computation) */
	static uint8_t CMD_RUN_KERNEL; 		// Run the previously offloaded kernel
	static uint8_t CMD_DEMO;			// Run a preprogrammed Vector Addition: R3 = R1 + R2

	/* 
	  Direct communication with vector registers of PIM.
	  Size : PIM_VREG_SIZE x (8b)
	*/
	void write_vreg( uint8_t *value ); 	// Write a full vector to VR  [HOST->PIM]
	void read_vreg ( uint8_t *value ); 	// Read a full vector from VR [PIM->HOST]

	/* 
	  Direct communication with scalar registers of PIM.
	  Size : 32b
	  Count: PIM_SREG_COUNT
	*/
	void write_sreg( unsigned i, ulong_t value ); 	// Write a value to SR[i]  [HOST->PIM]
	ulong_t read_sreg( unsigned i ); 				// Read a value from SR[i] [PIM->HOST]

    /*ADDED BY JIWON*/
    void write_info_bits( uint16_t infobits, int request_slot_number ); // writes the info bits to specified request slot [HOST->PIM]
    void write_skiplist_info_bits( uint8_t infobits, int request_slot_number ); // writes the info bits to specified request slot [HOST->PIM]
    void write_skiplist_random_level( uint8_t random_level, int request_slot_number ); // writes the random level necessary for skiplist add operation [HOST->PIM]
    uint16_t read_info_bits( int request_slot_number ); // reads the info bits from specified request slot [PIM->HOST]
    uint8_t read_skiplist_info_bits( int request_slot_number ); // reads the info bits from specified request slot [PIM->HOST]
    uint16_t read_timestamp( int request_slot_number ); // reads the operation timestamp from specified request slot [PIM->HOST]
    void write_parameter( ulong_t parameter, int request_slot_number ); // writes parameter to specified request slot [HOST->PIM]
    ulong_t read_parameter( int request_slot_number ); // reads parameter from specified request slot [PIM->HOST]
    /*end of ADDED BY JIWON*/


    /* ADDED BY JIWON, scrypt-specific functions */
    void write_scrypt_block( uint8_t *block, size_t blocksize, ulong_t alignment );
    void read_scrypt_block( uint8_t *block, size_t blocksize, ulong_t alignment );
    /* end of ADDED BY JIWON */

	/*
	Communicating with gem5 (for debugging)
	*/
	static uint8_t M5_ENABLE_PACKET_LOG;
	static uint8_t M5_DISABLE_PACKET_LOG;
	static uint8_t M5_TIME_STAMP;
	static uint8_t M5_EXIT;
	void give_m5_command(uint8_t command);			// Give a command to gem5 (for debugging only)
    void report_statistics();                       // Report software statistics
    void record_time_stamp(uint8_t ID);             // Record timestamp of gem5 and reset stats

protected:

	inline void wake_up();  						// Wakeup from sleep
	inline void give_command(uint8_t command);		// Write to PIM_COMMAND_REG
	inline uint8_t check_status();					// Read from PIM_STATUS_REG

	// Write and Read to/from the PIM device's registers
	inline void write_byte(ulong_t offset, uint8_t data);
	inline uint8_t read_byte(ulong_t offset);
	
private:

	// Open the PIM device as a file
	void open_device();

	//Map the whole physical range to userspace
	void mmap_device();

	// Pointer to the mmapped region of the PIM device
	char* pim_va;

	// File descriptor the PIM device
	int pim_fd;

    // PIM ID number (JIWON)
    int pim_id;

	// Arguments to send to ioctl (ioctl_arg[0] = length)
	ulong_t* ioctl_arg;

	// List of current tasks
	vector<PIMTask*> tasks;

    /* statistics */
    ulong_t stat_offload_size;   // Size of the offloaded binary (B)

    #ifdef DEBUG_API
    unsigned current_time_stamp; // (only in debug mode)
    #endif
};

#endif // _PIM_API_H_
