#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdlib.h>
#include "_app_params.h"
#include "app_utils.hh"
using namespace std;

#define TYPE ulong_t

TYPE* A;

void create_array()
{
    A = (TYPE*)allocate_region(ARRAY_SIZE*sizeof(TYPE));
    #ifdef INITIALIZE_ARRAY
    cout << "Initializing array[i] to ..." << endl;
    for ( unsigned i=0; i< ARRAY_SIZE; i++)
        A[i] = i;
    #else
    cout << "array is uninitialized!" << endl;
    #endif
}

ulong_t golden()
{
    ulong_t checksum = 0;
    for ( unsigned j=0; j< NUM_TURNS; j++ )
        for ( unsigned i=0; i<ARRAY_SIZE; i+=WALK_STEP )
        {
            checksum += A[i];
            // cout << "INDEX=" << i << endl;;
        }
    return checksum;
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    init_region();
    PIMAPI *pim = new PIMAPI(); /* Instatiate the PIM API */ 
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    create_array();

     cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    ____TIME_STAMP(1);
    #ifdef OFFLOAD_THE_KERNEL
    pim->offload_kernel((char*)FILE_NAME);
    #else
    cout << "Kernel offloading skipped! (See the OFFLOAD_THE_KERNEL environment variable)" << endl;
    #endif
    ____TIME_STAMP(2);

    /* PIM Scalar Registers
    SREG[0]: Array
    SREG[1]: Size
    SREG[2]: Step
    SREG[3]: NumTurns
    SREG[4]: Success (returned from PIM)
    */

    ____TIME_STAMP(3);
	// Create and assign the data set for this task
	PIMTask* task = new PIMTask("array-walk-task");
	task->addData(A, ARRAY_SIZE*sizeof(TYPE), 0);
    pim->write_sreg(1, ARRAY_SIZE);  // This is a parameter sent to PIM
    pim->write_sreg(2, WALK_STEP);  // This is a parameter sent to PIM
    pim->write_sreg(3, NUM_TURNS);  // This is a parameter sent to PIM

 
    cout << "(main.cpp): Offloading the task ... " << endl;
	pim->offload_task(task);
    ____TIME_STAMP(4);

   /* Enable packet logging */
    // pim->give_m5_command(PIMAPI::M5_ENABLE_PACKET_LOG);

    cout << "(main.cpp): Start computation ... " << endl;
    ____TIME_STAMP(5);
	pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
	pim->wait_for_completion();
    //____TIME_STAMP(6); // This is done in the resident code

    /* Disable packet logging */
    // pim->give_m5_command(PIMAPI::M5_DISABLE_PACKET_LOG);

    APP_INFO("[---DONE---]")

    cout << "(main.cpp): Running the golden model ... " << endl;
    ____TIME_STAMP(7);
    golden();
    ____TIME_STAMP(8);


    if ( pim->read_sreg(4) == 0 ) // Success
        print_happy();
    else
        cout << "Error! something went wrong ..." << endl;

    pim->report_statistics();
    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    // cout << "(main.cpp): Exiting main application!" << endl;
    delete pim;
	return 0;
}

