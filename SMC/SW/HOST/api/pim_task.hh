#ifndef _PIM_TASK_H_
#define _PIM_TASK_H_

// Headers
#include <stdlib.h>		// atoi, strtol
#include <vector>
#include <string>
#include <sstream>
#include "pim_utils.hh"
using namespace std;

/*****************************************************/
// Data to be offloaded to PIM (virtual address)
struct PIMData
{
	// Pointer to actual data and its size
	ulong_t		v_addr;			// Pointer to data (user virtual address)
	ulong_t		size;			// Size of the data (bytes)
	ulong_t		reg;			// Index of PIM SREG to point to hold a virtual ptr to this data (0, 1, ...)
};

/*****************************************************/
// Pages occupied by this PIMTask (virtual address)
struct PIMPage
{
	// Pointer to the first page allocated to this task
	ulong_t       v_pn_start;	// virtual page number of the first page
	ulong_t       v_pn_end;		// virtual page number of the last page
	ulong_t       count;			// Number of pages 
};


/*****************************************************/
// Task to be offloaded to PIM
class PIMTask
{
public:
	// Constructor
	PIMTask(string name);
	~PIMTask();

	/*
	  Add a data set to this task (sorts data on the fly based on addr)
	  @addr: pointer to user data (virtual address)
	  @size: size of the data in bytes
	  @reg: index of PIM's SREG to point to this data set
	*/
	void addData(void* addr, unsigned long size, unsigned reg);

	// Recalculate the allocated pages and merge the adjacent ones
	void updatePages();

	// Print the contents of this task
	void print();

	// Get the name of this task
	const char* getName();

	// The data sets allocated by this task (Sorted List)
	vector<PIMData*> 	data;

	// The virtual pages occupied by this task (Sorted List)
	vector<PIMPage*> 	vpages;

	// Page size of the OS
	static unsigned PAGE_SIZE;

	// Page shift of the OS (2**PAGE_SHIFT = PAGE_SIZE)
	static unsigned PAGE_SHIFT;

	// Convert Bytes to Kilo, Mega, ... Bytes
	static string MEM_SIZE(ulong_t value);

	// Convert page to string
	static string TO_STRING(PIMPage* page);

    /* statistics */
    ulong_t stat_datasize; // Total data size

private:
	// Name of the task
	string				name;

	// Pages have been updated or not
	bool _updated;
};


#endif // _PIM_TASK_H_
