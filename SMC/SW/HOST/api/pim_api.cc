#include "pim_api.hh"
#include "definitions.h"
#include <unistd.h>		// getpagesize
#include <string.h>		// memcpy
#include "defs.h"

uint8_t  PIMAPI::CMD_DEMO = PIM_COMMAND_DEMO;
uint8_t  PIMAPI::CMD_RUN_KERNEL = PIM_COMMAND_RUN_KERNEL;
uint8_t  PIMAPI::M5_ENABLE_PACKET_LOG = PIM_ENABLE_PACKET_LOG;
uint8_t  PIMAPI::M5_DISABLE_PACKET_LOG = PIM_DISABLE_PACKET_LOG;
uint8_t  PIMAPI::M5_TIME_STAMP = PIM_TIME_STAMP;
uint8_t  PIMAPI::M5_EXIT = PIM_EXIT_GEM5;

//*********************************************
PIMAPI::PIMAPI()
: pim_va(NULL), pim_fd(-1)
{
	API_INFO("Written by Erfan Azarkhish - Debugging Enabled");
	open_device();
	mmap_device();
    /* Reset statistics */
    stat_offload_size = 0;
	PIMTask::PAGE_SIZE = getpagesize();
	/* Calculate page shift based on page size */
	unsigned p=PIMTask::PAGE_SIZE;
	PIMTask::PAGE_SHIFT=0;
	while (p>1)
	{
		p>>=1;
		PIMTask::PAGE_SHIFT++;
	}
	ioctl_arg = new ulong_t[PIM_IOCTL_MAX_ARGS];
	ASSERT_DBG(ioctl_arg != NULL);
	// Common checks
    ASSERT_DBG( sizeof(ulong_t) == sizeof(void*) );
    ASSERT_DBG( PIM_SREG != 0 );
	ASSERT_DBG( PIM_VREG != 0 );
	ASSERT_DBG( CMD_DEMO == 'D' );
	ASSERT_DBG( CMD_RUN_KERNEL == 'K' );
	ASSERT_DBG( PIMTask::PAGE_SIZE != 0 );
	ASSERT_DBG( PIMTask::PAGE_SHIFT != 0 );
	API_INFO("OS: PAGE_SIZE=%d  PAGE_SHIFT=%d", PIMTask::PAGE_SIZE, PIMTask::PAGE_SHIFT);
}

//*********************************************
PIMAPI::~PIMAPI()
{
	for (unsigned j=0; j<tasks.size(); j++)
	{
		delete tasks[j];
	}
	API_INFO("API was killed successfully!");
}

//*********************************************
void PIMAPI::open_device()
{
	ASSERT_DBG( pim_fd == -1 );
	// open the pim device
	pim_fd = open("/dev/PIM", O_RDWR | O_SYNC ); // | O_DIRECT);
	if (pim_fd < 0)
	{
		perror("Open /dev/PIM");
		throw std::exception();
	}
	API_INFO("PIM device file opened successfully");
}

//*********************************************
void PIMAPI::mmap_device()
{
	ASSERT_DBG( pim_va == NULL );
	ASSERT_DBG( pim_fd != -1 );
	// Map the whole physical range to userspace
	pim_va = (char*)mmap(NULL, PHY_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, pim_fd, PHY_BASE);
	
	// Check if the memory map has failed
	if(pim_va == (char*) -1)
	{
		API_ALERT("Memory map failed");
		perror("mmap");
		throw std::exception();
	}
	else
	{
		API_INFO("Memory mapped at address %lx ", (unsigned long)pim_va);
	}
}

//*********************************************
void PIMAPI::start_computation(uint8_t command)
{
    ASSERT_DBG(current_time_stamp==5)
	API_INFO("STARTING COMPUTATION ...");
	give_command(command);
	wake_up();
}

//*********************************************
void PIMAPI::wait_for_completion()
{
    ASSERT_DBG(current_time_stamp==5)
	uint8_t status = 0;
	timespec short_sleep, sleep_rem;
	short_sleep.tv_sec=0;
	short_sleep.tv_nsec=50;

	do 
	{
		nanosleep(&short_sleep, &sleep_rem);
		status = check_status();
		API_INFO("PIM_STATUS_REG=%c", status );
	}
	while ( status != PIM_STATUS_DONE );

	// API_INFO("NOTICE: WE ARE RELEASING EVERYTHING! BUT THIS IS NOT GOOD, LATER ONLY RELEASE WHEN NECESSARY");
	// /*
	// Contact the driver to:
	// Release the page cache and free the pages
	// */
	// long success = ioctl(pim_fd,PIM_IOCTL_RELEASE_ALL_DATA, ioctl_arg);
	// if ( !success )
	// {
	// 	API_ALERT("Passing arguments to ioctl was not successful! TODO add retry later");
	// 	return;
	// }	
	// else if ( success < 0 )
	// {
	// 	API_ALERT("Error happened in ioctl: %ld", success);
	// 	return;
	// }

	API_INFO("GIVING NOP ...");
	give_command(PIM_COMMAND_NOP);
}

//*********************************************
void PIMAPI::write_vreg( uint8_t* value )
{
	ASSERT_DBG(PIM_VREG+PIM_VREG_SIZE < PHY_SIZE);
	memcpy(pim_va+PIM_VREG, value, PIM_VREG_SIZE);
}

//*********************************************
void PIMAPI::read_vreg( uint8_t* value )
{
	ASSERT_DBG(PIM_VREG+PIM_VREG_SIZE < PHY_SIZE);
	memcpy(value, pim_va+PIM_VREG, PIM_VREG_SIZE);
}

//*********************************************
void PIMAPI::write_sreg( unsigned index, ulong_t value )
{
    ASSERT_DBG(current_time_stamp==3)
	API_INFO("  SREG[%d] = 0x%lx", index, value );
	((ulong_t*)(pim_va+PIM_SREG))[index] = value;
}

//*********************************************
ulong_t PIMAPI::read_sreg( unsigned index )
{
	return ((ulong_t*)(pim_va+PIM_SREG))[index];
}

//*********************************************
void PIMAPI::write_byte(ulong_t offset, uint8_t data)
{
	ASSERT_DBG( offset < PHY_SIZE );
	*((uint8_t*)(pim_va+offset)) = data;
}

//*********************************************
uint8_t PIMAPI::read_byte(ulong_t offset)
{
	ASSERT_DBG( offset < PHY_SIZE );
	return *((uint8_t*)(pim_va+offset));
}

//*********************************************
uint8_t PIMAPI::check_status()
{
	return read_byte(PIM_STATUS_REG);
}

//*********************************************
void PIMAPI::give_command(uint8_t command)
{
	write_byte(PIM_COMMAND_REG, command);
}

//*********************************************
void PIMAPI::give_m5_command(uint8_t command)
{
	write_byte(PIM_M5_REG, command);
}

//*********************************************
void PIMAPI::wake_up()
{
	write_byte(PIM_INTR_REG, PIM_INTR_TRIGGER); // Value is ignored
}

//*********************************************
void PIMAPI::offload_kernel(char* name)
{
    ASSERT_DBG(current_time_stamp==1)
	API_INFO("Offloading kernel: %s", name);
	std::ifstream kf(name);
	std::string line, term;
	ulong_t load_offset=-1;
    #ifdef DEBUG_API
    ulong_t load_size=-1;
    #endif
	ulong_t num_bytes = 0;
	uint8_t value = 0;
	
	while ( std::getline( kf, line ) )
	{
		std::istringstream iss(line);
		while (iss >> term)
		{
			if ( term == "@ADDR" )
			{
				iss >> term;
				load_offset = atoi(term.c_str());
				std::cout << "@ADDR: " << term << " ";
			}
			else
			if ( term == "@SIZE" )
			{
				iss >> term;
                #ifdef DEBUG_API
				load_size = atoi(term.c_str());
                #endif
				std::cout << "@SIZE: " << term << " ";
			}
			else
			if ( term == "@END" )
			{
				ASSERT_DBG( num_bytes == load_size );
				num_bytes=0;
				std::cout << "@END" << std::endl;
			}
			else
			{
				value = strtol(term.c_str(), NULL, 16);
// 				// Compare
// 				char READ = read_byte(load_offset);
// 				if( value != READ )
// 				{
// 					cout << std::hex << (int)value;
// 					cout << " " << std::hex << (int)READ << endl;
// 				}
				write_byte(load_offset, value);
                stat_offload_size++;
				num_bytes++;
				load_offset++;
			}
		}
	}
	API_INFO("Offloading completed.");
}

//*********************************************
void PIMAPI::offload_task(PIMTask* task)
{
    ASSERT_DBG(current_time_stamp==3)
	API_INFO("Offloading task [%s] to the PIM device", task->getName());
	tasks.push_back(task);
	task->updatePages();
	task->print();
	ulong_t count = task->vpages.size();
	ASSERT_DBG(count*2 < PIM_IOCTL_MAX_ARGS);

	/*
	The virtual address range must be contiguous, so in the user application,
	we must make sure that all our data structures fall in the same virtual page range
	 */
	ASSERT_DBG(count == 1);

	/* Store the pointers and sizes in the ioctl_arg structure */
	ioctl_arg[0] = count*2;
	ulong_t i=0;
	while ( i < count )
	{
		ioctl_arg[2*i+1] = (task->vpages[i]->v_pn_start) << PIMTask::PAGE_SHIFT; // Virtual Address (Aligned to page borders)
		ioctl_arg[2*i+2] = task->vpages[i]->count;		// Page Count
		i++;
	}

	/*
	  Send pointers to this task's data sets to PIM via its scalar
	  registers (SREG). This way, PIM will be able to access these
	  data sets and work on them.
	  Important: here we do a simple address translation to map
	  host's virtual address to PIM's virtual address by simply
	  adding PIM_HOST_OFFSET to it
	*/
	API_INFO("Sending data pointers to PIM ...");
	for (unsigned i=0; i<task->data.size(); i++)
	{
		unsigned long translated = (unsigned long)task->data[i]->v_addr; // + PIM_HOST_OFFSET;		(REMOVED BY ERFAN)
		write_sreg(task->data[i]->reg, translated );
		//API_INFO("  SREG[%d] = 0x%x = Pointer to Data[%d]", task->data[i]->reg, translated, i);
	}	

	/* 
	  Contact the driver to:
	  Pin these pages into the memory
	  Create a SliceTable based on these pages
	  Send a pointer to the SliceTable to PIM
	 */
	long success = ioctl(pim_fd,PIM_IOCTL_ALLOCATE_DATA, ioctl_arg);
	if ( !success )
	{
		API_ALERT("Passing arguments to ioctl was not successful! TODO add retry later");
		return;
	}	
	else if ( success < 0 )
	{
		API_ALERT("Error happened in ioctl: %ld", success);
		return;
	}

	API_INFO("Task was offloaded successfully.");
}

void PIMAPI::report_statistics()
{
    cout << "********************* API STATISTICS ******************" << endl;

    cout <<"softwarestat.kernel.offloadsize\t" << stat_offload_size << " (B)" << endl;
    cout <<"softwarestat.task.datasize\t" << tasks[0]->stat_datasize << " (B)" << endl;
    cout <<"softwarestat.task.pagesize\t" << tasks[0]->vpages[0]->count * PIMTask::PAGE_SIZE << " (B)" << endl;
    cout <<"softwarestat.task.overhead\t" << (((float)(tasks[0]->vpages[0]->count*PIMTask::PAGE_SIZE)/(float)(tasks[0]->stat_datasize))-1.0)*100.0 << " (Percent)" << endl;

    long success = ioctl(pim_fd,PIM_IOCTL_REPORT_STATS, 0);
    if ( success <=0 )
    {
        API_ALERT("Error happened in ioctl: (PIM_IOCTL_REPORT_STATS) %ld", success);
        return;
    }
}

//*********************************************
void PIMAPI::record_time_stamp(uint8_t ID)
{
    #ifdef DEBUG_API
    current_time_stamp = ID;
    #endif
    give_m5_command(PIMAPI::M5_TIME_STAMP+ID);
}



