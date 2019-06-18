#include "kernel_params.h"

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
        /* PIM Scalar Registers
        SREG[0]: Array
        SREG[1]: Size
        SREG[2]: Step
        SREG[3]: NumTurns
        SREG[4]: Success (returned from PIM)
        */

        ulong_t* A;
        ulong_t Step, NumTurns, ASize;
        ulong_t checksum;
        ulong_t i, j;

        A = (ulong_t*) PIM_SREG[0];
        ASize = PIM_SREG[1]; 
        Step = PIM_SREG[2];
        NumTurns = PIM_SREG[3];
        PIM_SREG[4] = 1;

        pim_print_hex("Array", (ulong_t) A );
        pim_print_hex("Size", ASize );
        pim_print_hex("Step", Step );
        pim_print_hex("NumTurns", NumTurns);

        checksum = 0;
        for ( j=0; j< NumTurns; j++ )
            for ( i=0; i<ASize; i+=Step )
            {
    //            DTLB_REFILL_ideal(&A[i]);
                checksum += A[i];
            }
        PIM_SREG[5] = checksum; // Success
        PIM_SREG[4] = 0;
    //    pim_print_msg("Array Walk...");
    }

#endif