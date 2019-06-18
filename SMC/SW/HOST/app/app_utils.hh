#ifndef APP_UTILS_H
#define APP_UTILS_H

#include "_app_params.h"

/********************************
Utilities to be used by all apps
********************************/

#define ulong_t unsigned long       // 32b on ARMv8, 64b on ARMv8

/*****************************************************/
// Messages and Debugging facilities
#ifdef DEBUG_APP
#define APP_INFO(fmt,args...)       {printf("[PIM.APP]: " fmt "\n", ## args);}
#else
#define APP_INFO(fmt,args...)       // Do nothing
#endif

#define ____TIME_STAMP(X)     {pim->record_time_stamp(X);}

/* 
****************************
Memory Allocation Facilities
****************************
This util has several benefits:
1. makes sure that the allocated region is virtually contiguous (better for the driver)
2. there is no need for address translation inside PIM anymore
* PHY_SIZE: Reserved space for PIM to avoid virtual address translation between host and PIM
* REQUIRED_MEM_SIZE: Dynamically allocated memory to be shared with PIM
* SMC_BURST_SIZE_B: Align the allocated memory to SMC Burst
*/
#define MAX_REGION_SIZE (PHY_SIZE+REQUIRED_MEM_SIZE+SMC_BURST_SIZE_B)
volatile char  region_raw[MAX_REGION_SIZE];       // Continuously allocated region
volatile char* region_aligned;                    // Pointer to the next free memory location
volatile char* region_end;                        // End of the region


// Initialize the memory region to be shared with PIM
void init_region()
{
    region_aligned = (char*)&region_raw + PHY_SIZE;
    region_end = (char*)&region_raw + MAX_REGION_SIZE;
    ulong_t mod = ((ulong_t)region_aligned) % SMC_BURST_SIZE_B;
    if ( mod != 0 )
        region_aligned = region_aligned + SMC_BURST_SIZE_B - mod;
    ASSERT_DBG( ((ulong_t)region_aligned) % SMC_BURST_SIZE_B == 0 ); // Cache aligned region
    ASSERT_DBG(region_aligned < region_end);
    cout << "$ Allocated Static Region: " << dec << MAX_REGION_SIZE << " Bytes. Address: 0x" << hex << (unsigned long)region_raw
         << " Aligned Address: 0x" << (unsigned long)region_aligned << dec << endl;
    cout << "$ Memory check ..." << endl;
    ASSERT_DBG((ulong_t)(region_aligned) >= PIM_HOST_OFFSET );
    cout << "$ Done!" << endl;
    srand(0);
}

// Allocate the memory region(similar to malloc) - Bytes
void* allocate_region(unsigned long size)
{
    ASSERT_DBG(size < (unsigned long)0x00000000FFFFFFFF);
    ASSERT_DBG(region_aligned+size < region_end);
    volatile char* curr = region_aligned;
    region_aligned += size;
    //cout << "$ ALLOCATED: [" << hex << (unsigned long)curr << ", " << (unsigned long)curr+size-1 << dec << "] Size: " << size << " Bytes" << endl;
    return (void*)curr;
}

void print_happy()
{
        cout << endl;
        cout << "             OOOOOOOOOOO" << endl;
        cout << "         OOOOOOOOOOOOOOOOOOO" << endl;
        cout << "      OOOOOO  OOOOOOOOO  OOOOOO" << endl;
        cout << "    OOOOOO      OOOOO      OOOOOO" << endl;
        cout << "  OOOOOOOO  #   OOOOO  #   OOOOOOOO" << endl;
        cout << " OOOOOOOOOO    OOOOOOO    OOOOOOOOOO" << endl;
        cout << "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO" << endl;
        cout << "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO" << endl;
        cout << "OOOO  OOOOOOOOOOOOOOOOOOOOOOOOO  OOOO" << endl;
        cout << " OOOO  OOOOOOOOOOOOOOOOOOOOOOO  OOOO" << endl;
        cout << "  OOOO   OOOOOOOOOOOOOOOOOOOO  OOOO" << endl;
        cout << "    OOOOO   OOOOOOOOOOOOOOO   OOOO" << endl;
        cout << "      OOOOOO   OOOOOOOOO   OOOOOO" << endl;
        cout << "         OOOOOO         OOOOOO" << endl;
        cout << "             OOOOOOOOOOOO" << endl;
        cout << endl;
}

#endif