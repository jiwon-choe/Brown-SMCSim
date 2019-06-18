#include "kernel_params.h"

/******************************************************************************/
#ifdef kernel_default

    void execute_kernel()
    {
    	/*
    	SREG[0]: A Matrix (Virtual Pointer)
    	SREG[1]: B Matrix (Virtual Pointer)
    	SREG[2]: C Matrix (Virtual Pointer)
    	SREG[3]: Size
    	Matrices are squared [Size x Size]
    	Each element is a ulong_t
    	*/
    	ulong_t *A, *B, *C;
    	ulong_t S, SS;
    	ulong_t i;

    	A = (ulong_t*) PIM_SREG[0];
    	B = (ulong_t*) PIM_SREG[1];
    	C = (ulong_t*) PIM_SREG[2];
    	S = PIM_SREG[3];

    	//pim_print_msg("<<< MATRIX ADDITION KERNEL >>>");
    	pim_print_hex("S", S );
    	pim_print_hex("A", (ulong_t)A );
    	pim_print_hex("B", (ulong_t)B );
    	pim_print_hex("C", (ulong_t)C );

    	SS = S*S;
    	for (i=0; i<SS; i++)
    	{
    		C[i] = A[i] + B[i];
    	}
    	pim_print_msg("Matrix addition finished ...");
    }

#endif

/******************************************************************************/
#ifdef kernel_best

    #define A_ping (ping+XFER_SIZE*0)
    #define B_ping (ping+XFER_SIZE*1)
    #define C_ping (ping+XFER_SIZE*2)
    #define A_pong (pong+XFER_SIZE*0)
    #define B_pong (pong+XFER_SIZE*1)
    #define C_pong (pong+XFER_SIZE*2)

    void execute_kernel()
    {
        /*
        SREG[0]: A Matrix (Virtual Pointer)
        SREG[1]: B Matrix (Virtual Pointer)
        SREG[2]: C Matrix (Virtual Pointer)
        SREG[3]: Size
        Matrices are squared [Size x Size]
        Each element is a ulong_t

        Notice: because of the DMA transfers, A, B, and C must be uint8_t* and not ulong_t*

        */
        uint8_t *A, *B, *C;    // Notice: because of the DMA transfers, A, B, and C must be uint8_t* and not ulong_t*
        ulong_t S, SS;
        ulong_t x, j;
        ulong_t num_bursts;

        // Ping-pong buffering
        volatile uint8_t* ping;
        volatile uint8_t* pong;
        volatile uint8_t* swap;


        A = (uint8_t*) PIM_SREG[0];
        B = (uint8_t*) PIM_SREG[1];
        C = (uint8_t*) PIM_SREG[2];
        S = PIM_SREG[3];

        pim_print_hex("S", S );
        pim_print_hex("A", (ulong_t)A );
        pim_print_hex("B", (ulong_t)B );
        pim_print_hex("C", (ulong_t)C );

        SS = S*S;
        pim_print_hex("SS", SS );
        ping = &PIM_VREG[0];
        pong = &PIM_VREG[0] + MAX_XFER_SIZE*3;

        // First we have to fill the ping buffer (only once)
        DMA_REQUEST(A, A_ping, XFER_SIZE, PIM_DMA_READ, DMA_RES0 );
        DMA_REQUEST(B, B_ping, XFER_SIZE, PIM_DMA_READ, DMA_RES1 );
        DMA_WAIT( DMA_RES0);
        DMA_WAIT( DMA_RES1);

        num_bursts = SS*sizeof(ulong_t)/XFER_SIZE;
        for (x=0; x<num_bursts; x++)
        {
            // Fill Pong (Request)
            if ( x+1 < num_bursts ) // Boundary
            {
                DMA_REQUEST(A+(x+1)*XFER_SIZE, A_pong, XFER_SIZE, PIM_DMA_READ, DMA_RES2 );
                DMA_REQUEST(B+(x+1)*XFER_SIZE, B_pong, XFER_SIZE, PIM_DMA_READ, DMA_RES3 );
            }

            // Work on Ping's data
            for ( j=0; j< XFER_SIZE/sizeof(ulong_t); j++ )
                ((ulong_t*)C_ping)[j] = ((ulong_t*)A_ping)[j] + ((ulong_t*)B_ping)[j];

            // Write back the result of Ping
            DMA_WAIT( DMA_RES4);
            DMA_REQUEST(C+x*XFER_SIZE, C_ping, XFER_SIZE, PIM_DMA_WRITE, DMA_RES4 );

            // Wait for Pong to finish
            DMA_WAIT( DMA_RES2);
            DMA_WAIT( DMA_RES3);

            // Swap ping and pong
            swap = ping;
            ping = pong;
            pong = swap;
        }
        DMA_WAIT( DMA_RES4);
        //pim_print_msg("Matrix addition finished ...");
    }

#endif  
