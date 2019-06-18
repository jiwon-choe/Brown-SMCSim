
void execute_kernel()
{
    /* PIM Scalar Registers
    SREG[0]: Array
    SREG[1]: Size
    SREG[2]: Step
    SREG[3]: NumTurns
    SREG[4]: Success (returned from PIM)
    */

    char* A;
    ulong_t Step, NumTurns, ASize;
    ulong_t checksum;
    ulong_t i;

    A = (char*) PIM_SREG[0];
    ASize = PIM_SREG[1]; 
    Step = PIM_SREG[2];
    NumTurns = PIM_SREG[3];
    PIM_SREG[4] = 1;

    pim_print_msg("Executing a DMA request");


    pim_print_hex("Array", (ulong_t) A );
    pim_print_hex("Size", ASize );
    pim_print_hex("Step", Step );
    pim_print_hex("NumTurns", NumTurns);

    for ( i=0; i<8; i++ )
        pim_print_hex("Array=", A[i] );

    pim_print_msg("Starting a DMA transfer from A[] ...");
    DMA_REQUEST(A,      PIM_VREG,      2048, PIM_DMA_READ, DMA_RES0 );
    DMA_REQUEST(A+2048, PIM_VREG+2048, 2048, PIM_DMA_READ, DMA_RES1 );

    // We have two outstanding DMA requests
    DMA_WAIT( DMA_RES0) pim_print_hex("S", PIM_DMA_STATUS );
    DMA_WAIT( DMA_RES1) pim_print_hex("S", PIM_DMA_STATUS );

    // __asm__("wfi");
    // pim_print_msg("[WAKEUP]");

    for ( i=0; i<4096; i++ )
        pim_print_hex("After: VREG=", PIM_VREG[i] );
    
    checksum = 0;
    PIM_SREG[5] = checksum; // Success
    PIM_SREG[4] = 0;
}

