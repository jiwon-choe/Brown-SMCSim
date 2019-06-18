#include "pim_task.hh"
#include "definitions.h"

unsigned PIMTask::PAGE_SIZE = 0;
unsigned PIMTask::PAGE_SHIFT = 0;

//*********************************************
PIMTask::PIMTask(string _name)
{
	name = _name;
	_updated = false;
    stat_datasize = 0;
}

//*********************************************
PIMTask::~PIMTask()
{
	for (unsigned i=0; i<data.size(); i++)
		delete data[i];
	for (unsigned i=0; i<vpages.size(); i++)
		delete vpages[i];
}

//*********************************************
void PIMTask::addData(void* addr, unsigned long size, unsigned reg)
{
	#ifdef DEBUG_API
	/*
	Explanation: There must be no overlap between PIM's virtual address
	and host's virtual address. To avoid address translation overheads
	we don't maintain this anymore, and instead we allocate a reserved
	region for PIM inside the user application, and also check here
	for possible overlaps.
	Notice that this check is not enough in case of pointer based structs
	such as tree. But reserving a region for PIM makes it safe.
	*/
    API_INFO("Task.AddData Address:0x%lx  Size:%ld Reg:%d", (unsigned long)addr, size, reg);
	ASSERT_DBG( (ulong_t)addr>=PIM_HOST_OFFSET );
	#endif

	PIMData* d = new PIMData();
	d->v_addr = (ulong_t)addr;
	d->size = size;
    stat_datasize += size;
	d->reg = reg;
	/* Insert data in the list in ascending order of addr */
	for ( unsigned i=0; i<data.size(); i++ )
	{
		if ( data[i]->v_addr > d->v_addr )
		{
			data.insert(data.begin()+i, d);
			return;
		}
	}
	_updated = false;
	data.push_back(d);
}

//*********************************************
void PIMTask::print()
{
	#ifdef DEBUG_API
	ASSERT_DBG(_updated);
	API_INFO("-------------------------------------------------------------");
	API_INFO("Task: [%s] [DataCount: %ld]", name.c_str(), (unsigned long)data.size());
	ulong_t tot_data_size = 0;
	ulong_t tot_mem_size = 0;
	for (unsigned i=0; i<data.size(); i++)
	{
		API_INFO("   Data[%d]:  VA [0x%lx, 0x%lx]:%ld(B) ", i, (ulong_t)data[i]->v_addr,
		 (ulong_t)data[i]->v_addr+(ulong_t)data[i]->size-1, (ulong_t)data[i]->size);
		tot_data_size += data[i]->size;
	}
	for (unsigned i=0; i<vpages.size(); i++)
	{
		API_INFO("   Pages[%d]: VA [%lx, %lx]:%ld:%s", i, ((ulong_t)(vpages[i]->v_pn_start)) << PAGE_SHIFT,
		(((ulong_t)(vpages[i]->v_pn_end)) << PAGE_SHIFT) + PAGE_SIZE-1, vpages[i]->count, MEM_SIZE(vpages[i]->count*PAGE_SIZE).c_str());
		tot_mem_size += vpages[i]->count*PAGE_SIZE;
	}
	API_INFO("   Memory Overhead: %s %%%.1f", MEM_SIZE(tot_mem_size-tot_data_size).c_str(), 100.0*(float)(tot_mem_size-tot_data_size)/(float)(tot_mem_size));
	API_INFO("-------------------------------------------------------------");
	#endif
}

//*********************************************
const char* PIMTask::getName()
{
	return name.c_str();
}

//*********************************************
void  PIMTask::updatePages()
{
	API_INFO("Updating pages for task %s ...", getName());

	#ifdef DEBUG_API
	ulong_t prev_end_addr = (ulong_t)0;
	#endif
	vector<PIMData*>::iterator it;
	int i = 0; // counter.
	PIMPage* page = NULL;
	/* Try to merge the adjacent range with the existing ranges */
	for(it=data.begin() ; it < data.end(); it++, i++ )
	{
		PIMData* d = *it;
		ulong_t start_address = d->v_addr;
		ulong_t end_address = d->v_addr + d->size-1;
		ulong_t start_page = ((ulong_t)start_address >> PAGE_SHIFT);
		ulong_t end_page = ((ulong_t)end_address >> PAGE_SHIFT);
		//API_INFO("virtual region: start=%lx, end:%lx", start_page << PAGE_SHIFT, end_page << PAGE_SHIFT);

		if ( page )
		{
			// If this range overlaps with the next range
			if (page->v_pn_end >= start_page-1)
			{
				// Merge with the next region
				page->v_pn_end = end_page;
				page->count = page->v_pn_end - page->v_pn_start+1;
			}
			else
			{
				vpages.push_back(page);
				page = NULL;
			}
		}
		if ( page == NULL )
		{
			page = new PIMPage();
			page->v_pn_start = start_page;
			page->v_pn_end = end_page;
			page->count = end_page-start_page+1;
		}

		#ifdef DEBUG_API
		if ( prev_end_addr != 0  && prev_end_addr > start_address )
		{
			API_ALERT("Error: overlap between regions!");
			print();
			exit(1);
		}
		prev_end_addr = end_address;
		#endif
	}
	if ( page )
		vpages.push_back(page);
	_updated = true;
}

//*********************************************
string PIMTask::MEM_SIZE(ulong_t value)
{
	string unit = "(B)";
	if (value > 1024*1024)
	{	
		value /= (1024*1024);
		unit="(MB)";
	}
	else
	if (value > 1024)
	{	
		value /= (1024);
		unit="(KB)";
	}
	ostringstream oss;
	oss << value << unit;
	return oss.str();
}

//*********************************************
string PIMTask::TO_STRING(PIMPage* page)
{
	ostringstream oss;
	oss << "[" << hex << page->v_pn_start << ", "<< page->v_pn_end << "]:"<< MEM_SIZE(page->count*PAGE_SIZE);
	return oss.str();
}
