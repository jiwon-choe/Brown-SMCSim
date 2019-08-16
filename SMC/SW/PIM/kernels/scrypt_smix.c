/*
The included code and documentation ("scrypt") is distributed under the
following terms:

Copyright 2005-2016 Colin Percival.  All rights reserved.
Copyright 2005-2016 Tarsnap Backup Inc.  All rights reserved.
Copyright 2014 Sean Kelly.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.*/

/* some functions have been copied over from scrypt's crypto_scrypt_smix.c and sysendian.h */


#include "defs.hh"
#include "kernel_params.h"
#include <stdint.h>
#include <stdio.h>

// need to get from host:
// 1. block B -- to be more exact, the i-th block in B = (B0, B1, ..., Bp-1)
//               (function in PIM applies SMIX on the i-th block)
//               B has size 128r bytes. realistic r is 8. but may be increased.
//               final content of B is used for scrypt result, 
//               so final value needs to be communicated to host.
// 2. r value
// 3. N value
// (host code for crypto_scrypt_smix receives ptrs to memory blocks V and XY. However, this is only to pass on ptrs to pre-defined locations. All data in the blocks are initialized within the scrypt smix code, so this can all be set up in the PIM kernel.)


#define VAULT_BASE_ADDR 0xC0000000
#define FREE_MEM_PTR PIM_SREG[7]
#define FREE_VREG_PTR PIM_SREG[6]
#define DRAM_ROW_LENGTH 256 // USED TO ALIGN MEMORY
#define BLOCK_BASE_ADDR (uint8_t *)(&PIM_VREG[0]) // ptr to block B TODO: make sure that content is 64-byte aligned



void * allocate_memory(size_t memsize) {
    void * allocated_mem_addr = (void *)FREE_MEM_PTR;
    FREE_MEM_PTR = (FREE_MEM_PTR + memsize + DRAM_ROW_LENGTH - 1) & ~((ulong_t)255);
    return allocated_mem_addr;
}


/************************************************
* from sysendian.h                              *
*************************************************/

uint32_t pim_le32dec(const void *pp) {
	const uint8_t * p = (uint8_t const *)pp;

	return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
	    ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}

void pim_le32enc(void * pp, uint32_t x)
{
	uint8_t * p = (uint8_t *)pp;

	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
}

/************************************************
* from cryto_scrypt_smix.c                      *
*************************************************/

void blkcpy(void *dest, const void *src, size_t len) {
	size_t * D = (size_t *)dest;
	const size_t * S = (const size_t *)src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] = S[i];
}

void blkxor(void *dest, const void *src, size_t len) {
	size_t * D = (size_t *)dest;
	const size_t * S = (const size_t *)src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] ^= S[i];
}

uint64_t integerify(const void * B, size_t r)
{
	const uint32_t * X = (const uint32_t *)((uintptr_t)(B) + (2 * r - 1) * 64);

	return (((uint64_t)(X[1]) << 32) + X[0]);
}

void salsa20_8(uint32_t B[16])
{
	uint32_t x[16];
	size_t i;

	blkcpy(x, B, 64);
	for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
		x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);

		x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
		x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);

		x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
		x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);

		x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
		x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);

		/* Operate on rows. */
		x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
		x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);

		x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
		x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);

		x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
		x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);

		x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
		x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
#undef R
	}
	for (i = 0; i < 16; i++)
		B[i] += x[i];
}

void blockmix_salsa8(const uint32_t * Bin, uint32_t * Bout, uint32_t * X, size_t r)
{
	size_t i;

	/* 1: X <-- B_{2r - 1} */
	blkcpy(X, &Bin[(2 * r - 1) * 16], 64);

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < 2 * r; i += 2) {
		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8], X, 64);

		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16 + 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8 + r * 16], X, 64);
	}
}

/***********************************************************************/

/*
    SREG[0]: r
    SREG[1]: N
    SREG[2]: memory alignment
    SREG[3]: PIM ID
    SREG[6]: free vreg space pointer
    SREG[7]: free memory pointer (vault memory)
*/
void execute_kernel()
{
    FREE_MEM_PTR = VAULT_BASE_ADDR;
    size_t r = (size_t)PIM_SREG[0];
    ulong_t N = PIM_SREG[1];
    ulong_t mem_align = PIM_SREG[2];
    ulong_t pim_id = PIM_SREG[3];
    FREE_VREG_PTR = (ulong_t)(PIM_VREG + (mem_align-1)) & ~(ulong_t)(mem_align-1);
    uint32_t *V = (uint32_t *)allocate_memory(128*r*N);
    uint8_t *B = (uint8_t *)FREE_VREG_PTR;
    uint32_t *XY = (uint32_t *)(0x0000A000); // beginning of free scratchpad mem
    uint32_t *X = XY;
    uint32_t *Y = (uint32_t *)((uint8_t *)(XY) + 128*r);
    uint32_t *Z = (uint32_t *)((uint8_t *)(XY) + 256*r);
    size_t k = 0;
    ulong_t i = 0;
    uint64_t j = 0;
    uint32_t *V0 = (uint32_t *)FREE_VREG_PTR; // slot for V_i DMA-ed into vreg space (overwrite B in vreg in order to fully utilize vreg space for DMA)
    FREE_VREG_PTR += 128*r;


    // 1. initialize X
    for (k = 0; k < 32*r; k++) {
        X[k] = pim_le32dec(&B[4*k]);
    }

    // 2. initialize V
    // this part does not need DMA -- writes to memory take less time, because most of them stay queued.
    for (i = 0; i < N; i+= 2) {
		blkcpy(&V[i * (32 * r)], X, 128 * r); // dest, src, len
		blockmix_salsa8(X, Y, Z, r);
		blkcpy(&V[(i + 1) * (32 * r)], Y, 128 * r);
		blockmix_salsa8(Y, X, Z, r);
    }

    // 3. memory-hard mixing operations
	for (i = 0; i < N; i += 2) {
		j = integerify(X, r) & (N - 1);
        DMA_REQUEST(&V[j*(32*r)], V0, 128*r, PIM_DMA_READ, DMA_RES0);
        DMA_WAIT(DMA_RES0);
        blkxor(X, V0, 128*r);
		blockmix_salsa8(X, Y, Z, r);

		j = integerify(Y, r) & (N - 1);
        DMA_REQUEST(&V[j*(32*r)], V0, 128*r, PIM_DMA_READ, DMA_RES0);
        DMA_WAIT(DMA_RES0);
        blkxor(Y, V0, 128*r);
		blockmix_salsa8(Y, X, Z, r);
	}

    // 4. update B
	for (k = 0; k < 32 * r; k++) {
		pim_le32enc(&B[4 * k], X[k]);
    }
}
