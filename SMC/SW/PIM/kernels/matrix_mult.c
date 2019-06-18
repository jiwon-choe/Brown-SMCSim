#include "kernel_params.h"

// Return M[r][c] where M is an SxS matrix
#define ELEMENT(M, S, r, c)	M[r*S+c]

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
	*/
	ulong_t *A, *B, *C;
	ulong_t S, Sum;
	unsigned r, c, k;

	A = (ulong_t*) PIM_SREG[0];
	B = (ulong_t*) PIM_SREG[1];
	C = (ulong_t*) PIM_SREG[2];
	S = PIM_SREG[3];

	pim_print_msg("<<< MATRIX MULTIPLICATION KERNEL >>>");
	pim_print_hex("S", S );
	pim_print_hex("A", (ulong_t)A );
	pim_print_hex("B", (ulong_t)B );
	pim_print_hex("C", (ulong_t)C );

    for (r=0; r<S; r++)
        for (c=0; c<S; c++)
        {
            Sum = 0;
            for (k=0;k<S; k++)
                Sum += (ELEMENT(A, S, r, k) * ELEMENT(B, S, k, c));
            ELEMENT(C, S, r, c) = Sum;
        }

	pim_print_msg("Matrix multiplication finished ...");
}

