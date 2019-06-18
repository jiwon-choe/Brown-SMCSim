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
#define MATRIX_TYPE  TYPE(*)[SIZE]

/************************************************/
// Print the matrix to the screen as an array
void print_matrix_as_array(TYPE M[SIZE][SIZE], string name)
{
    cout << "Matrix " << name << ":" << endl;
    cout << "[ ";
    for ( ulong_t r=0; r<SIZE; r++ )
    {
        for ( ulong_t c=0; c<SIZE; c++ )
        {
            cout << (TYPE)M[r][c] << " ";
        }
    }
    cout << "]" << endl;
}

/************************************************/
// Print the matrix to the screen
void print_matrix(TYPE M[SIZE][SIZE], string name)
{
    ulong_t LIMIT = SIZE;
    if ( SIZE > 20 )
        LIMIT=20;

    cout << "Matrix " << name << ":" << endl;
    for ( ulong_t r=0; r<LIMIT; r++ )
    {
        cout << "[ ";
        for ( ulong_t c=0; c<LIMIT; c++ )
        {
            cout << setw(8) << (TYPE)M[r][c] << " ";
        }
        cout << "]" << endl;
    }
}

/************************************************/
// Golden Model: Z = X OP Y
void calculate_golden(TYPE X[SIZE][SIZE], TYPE Y[SIZE][SIZE], TYPE Z[SIZE][SIZE] )
{
    if ((strcmp(NAME, "matrix_add") == 0) || (strcmp(NAME, "matrix_add_dma") == 0))
    {
        for ( ulong_t r=0; r< SIZE; r++ )
            for ( ulong_t c=0; c< SIZE; c++ )
                Z[r][c] = X[r][c] + Y[r][c];
    }
    else if (strcmp(NAME, "matrix_mult") == 0)
    {
        for ( ulong_t r=0; r< SIZE; r++ )
            for ( ulong_t c=0; c< SIZE; c++ )
            {
                TYPE Sum = 0;
                for (ulong_t k=0;k<SIZE; k++)
                    Sum += X[r][k] * Y[k][c];
                Z[r][c] = Sum;
            }
    }
    else
    {
        cout << "Error: golden model not implemented!" << endl;
        exit(1);
    }
}

/************************************************/
// Compare X with Y
bool compare_golden(TYPE C[SIZE][SIZE], TYPE G[SIZE][SIZE] )
{
    bool flag = true;
    cout << "Comparing results with the golden model ..." << endl;
    for ( ulong_t r=0; r< SIZE; r++ )
        for ( ulong_t c=0; c< SIZE; c++ )
            if ( C[r][c] != G[r][c] )
            {
                cout << "ERROR: C["<<r<<"]["<<c<<"]="<<(TYPE)C[r][c] << "  !=   G["<<r<<"]["<<c<<"]="<<(TYPE)G[r][c] << endl;
                flag = false;
            }
    return flag;
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    init_region();
    PIMAPI *pim = new PIMAPI(); /* Instatiate the PIM API */
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << " Initializing the matrices ..." << endl;
    // Allocate the matrices
    void* A = allocate_region(SIZE*SIZE*sizeof(TYPE));
    void* B = allocate_region(SIZE*SIZE*sizeof(TYPE));
    void* C = allocate_region(SIZE*SIZE*sizeof(TYPE));
    void* G = allocate_region(SIZE*SIZE*sizeof(TYPE));

    // Initialize the matrices (deal with them as an array)
    for ( ulong_t i=0; i<(ulong_t)(SIZE*SIZE); i++ )
    {
        ((TYPE*)A)[i] = i;//rand();
        ((TYPE*)B)[i] = i;//rand();
        ((TYPE*)C)[i] = 0;
    }

    ____TIME_STAMP(1);
    #ifdef OFFLOAD_THE_KERNEL
	pim->offload_kernel((char*)FILE_NAME);
    #else
    cout << "Kernel offloading skipped! (See the OFFLOAD_THE_KERNEL environment variable)" << endl;
    #endif
    ____TIME_STAMP(2);
	
	/* PIM Scalar Registers
	SREG[0]: A Matrix (Virtual Pointer)
	SREG[1]: B Matrix (Virtual Pointer)
	SREG[2]: C Matrix (Virtual Pointer)
	SREG[3]: Size
	*/

    ____TIME_STAMP(3);
	// Create and assign the data set for this task
	PIMTask* task = new PIMTask("vector-add-task");
	task->addData(A, (SIZE*SIZE)*sizeof(TYPE), 0);
	task->addData(B, (SIZE*SIZE)*sizeof(TYPE), 1);
	task->addData(C, (SIZE*SIZE)*sizeof(TYPE), 2);
	pim->write_sreg(3, SIZE);  // This is a parameter sent to PIM

    /* Enable packet logging */
    //pim->give_m5_command(PIMAPI::M5_ENABLE_PACKET_LOG);

	pim->offload_task(task);
    ____TIME_STAMP(4);

    ____TIME_STAMP(5);
	pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
	pim->wait_for_completion();
    //____TIME_STAMP(6); // This is done in the resident code

    // Calculate the golden model
    cout << "(main.cpp): Building the golden model ..." << endl;
    ____TIME_STAMP(7);
    calculate_golden((MATRIX_TYPE)A, (MATRIX_TYPE)B, (MATRIX_TYPE)G);
    ____TIME_STAMP(8);


    /* Disable packet logging */
    //pim->give_m5_command(PIMAPI::M5_DISABLE_PACKET_LOG);

	APP_INFO("[---DONE---]");

    /*
    Print the results
    */
    // print_matrix((MATRIX_TYPE)A, "A");
    // print_matrix((MATRIX_TYPE)B, "B");
    // print_matrix((MATRIX_TYPE)C, "C");

    if ( ! compare_golden((MATRIX_TYPE)C, (MATRIX_TYPE)G) )
    {
        cout << "Error! not equal, dumping the matrices ..." << endl;
        print_matrix_as_array((MATRIX_TYPE)C, "C");
        print_matrix_as_array((MATRIX_TYPE)G, "G");
    }
    else
        print_happy();

    pim->report_statistics();
	cout << "(main.cpp): Exiting main application!" << endl;
	delete pim;
	return 0;
}

