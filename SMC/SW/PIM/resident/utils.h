#ifndef _PIM_DEVICE_UTILS_
#define _PIM_DEVICE_UTILS_

#include "registers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Refill the TLB (Ideal mode)
#define DTLB_REFILL_ideal(X)  {PIM_DTLB_IDEAL_REFILL=(ulong_t)X;}
#define HMC_ATOMIC___INCR(X)  {HMC_ATOMIC_INCR = (ulong_t)&((X));}
#define HMC_ATOMIC___IMIN(X)  {HMC_ATOMIC_IMIN = (ulong_t)&((X));}
#define HMC_ATOMIC___FADD(X)  {HMC_ATOMIC_FADD = (ulong_t)&((X));}

// DMA Request
#define DMA_REQUEST(MA,SA,NB,CM, RES) {PIM_DMA_MEM_ADDR=(ulong_t)MA; PIM_DMA_SPM_ADDR=(ulong_t)SA; PIM_DMA_NUMBYTES=(ulong_t)NB; PIM_DMA_COMMAND=((CM)|(RES));}
#define DMA_WAIT(RES) {while(PIM_DMA_STATUS & RES) {__asm__("wfi");PIM_DMA_CLI = 0xFF;}} 

typedef enum { false, true } bool;

// Forward Declarations
void execute_kernel();

//*************************************
void pim_exit(char error_code)
{
	PIM_ERROR_REG = PIM_ERROR_FAIL;
}

//*************************************
#ifdef DEBUG_RESIDENT
void pim_print_msg( char* msg )
{
	int i=0;
	PIM_DEBUG_REG = '[';PIM_DEBUG_REG = 'P';PIM_DEBUG_REG = 'I';PIM_DEBUG_REG = 'M';PIM_DEBUG_REG = ']';PIM_DEBUG_REG = ':';PIM_DEBUG_REG = ' ';
	while ( msg[i] != 0 )
	{
		PIM_DEBUG_REG = msg[i];
		i++;
	}
	PIM_DEBUG_REG = '\r';
	PIM_DEBUG_REG = '\n';
}
#else
#define pim_print_msg( msg ) {}  // Do nothing
#endif

//*************************************
void pim_error(char* msg)
{
	char rpt[100] = "(Error):";
	strcat(rpt, msg);
	pim_print_msg((char*)rpt);
	pim_exit(PIM_ERROR_FAIL);
}

//*************************************
void execute_demo()
{
	int i;
    // Increment all registers by 1
	for ( i=0; i<PIM_VREG_SIZE; i++)
		PIM_VREG[i] = PIM_VREG[i] + 1;
}

//*************************************
void pim_print_char( char* msg, char value )
{
	int i=0;
	PIM_DEBUG_REG = '[';PIM_DEBUG_REG = 'P';PIM_DEBUG_REG = 'I';PIM_DEBUG_REG = 'M';PIM_DEBUG_REG = ']';PIM_DEBUG_REG = ':';PIM_DEBUG_REG = ' ';
	while ( msg[i] != 0 )
	{
		PIM_DEBUG_REG = msg[i];
		i++;
	}
	PIM_DEBUG_REG = '=';
	PIM_DEBUG_REG = value;
	PIM_DEBUG_REG = '\r';
	PIM_DEBUG_REG = '\n';
}

//*************************************
#ifdef DEBUG_RESIDENT
void pim_print_hex( char* msg, ulong_t value )
{
	int i=0;
	ulong_t v;
	char num[8];
	PIM_DEBUG_REG = '[';PIM_DEBUG_REG = 'P';PIM_DEBUG_REG = 'I';PIM_DEBUG_REG = 'M';PIM_DEBUG_REG = ']';PIM_DEBUG_REG = ':';PIM_DEBUG_REG = ' ';
	while ( msg[i] != 0 )
	{
		PIM_DEBUG_REG = msg[i];
		i++;
	}
	PIM_DEBUG_REG = '=';PIM_DEBUG_REG = '0';PIM_DEBUG_REG = 'x';
	for (i=SIZEOF_ULONG*2-1; i>=0; i--)
	{
		v = value % 16;
		if (v <= 9)
			num[i] = v + '0';
		else
			num[i] = v + 'A' - 10;
		value /= 16;
	}
	for (i=0; i<8; i++)
	{
		PIM_DEBUG_REG = num[i];
	}
	PIM_DEBUG_REG = '\r';
	PIM_DEBUG_REG = '\n';
}
#else
#define pim_print_hex( msg, value ) {}	// Do nothing
#endif

//*************************************
#ifdef DEBUG_RESIDENT
void pim_assert(bool condition, char* msg)
{
	if ( !condition )
	{
		char rpt[100] = "(Assertion Failed):";
		strcat(rpt, msg);
 		pim_print_msg((char*)rpt);
		pim_exit(PIM_ERROR_FAIL);
	}
}
#else
#define pim_assert(condition,msg) {} // Do nothing
#endif

//*************************************
void pim_wait(long value)
{
	long i=0;
	for ( i=0; i<value; i++);
}

//*************************************
void initialize_registers()
{
    /*
    We have to initialize the registers here, otherwise they may be removed by the compiler (if they are only used in the computation kernel)
    */
    unsigned i;
	pim_assert(sizeof(void*)==SIZEOF_ULONG, "Pointer must be compatible with SIZEOF_ULONG");
	pim_assert(sizeof(ulong_t)==SIZEOF_ULONG, "ulong_t must be compatible with SIZEOF_ULONG");
	PIM_STATUS_REG = PIM_STATUS_SLEEP;
	PIM_COMMAND_REG = PIM_COMMAND_NOP;
	PIM_ERROR_REG = PIM_ERROR_NONE;
    for ( i=0; i< PIM_VREG_SIZE; i++ )
    {
        PIM_VREG[i]=0;
    }
    for ( i=0; i< PIM_SREG_COUNT; i++ )
        PIM_SREG[i] = 0;

    PIM_SLICETABLE = 0;
    PIM_SLICECOUNT = 0;
    PIM_SLICEVSTART = 0;
    PIM_DTLB_IDEAL_REFILL = 0; // Null Pointer
    PIM_M5_D1_REG = 0;
    PIM_M5_D2_REG = 0;
    PIM_INTR_REG = PIM_INTR_NOP;
    PIM_M5_REG= PIM_M5_IGNORE;

    // Initialize DMA registers
    PIM_DMA_MEM_ADDR = 0;
    PIM_DMA_SPM_ADDR = 0;
    PIM_DMA_NUMBYTES = 0;
    PIM_DMA_COMMAND = 0; // NOP
    PIM_DMA_STATUS = 0; // All Free
    PIM_DMA_CLI = 0; // No wait
    HMC_ATOMIC_INCR = 0; // NOP
    HMC_ATOMIC_IMIN = 0; // NOP
    HMC_ATOMIC_FADD = 0; // NOP
    HMC_OPERAND = 0;

    PIM_COPROCESSOR_CMD = 0;

    pim_print_hex("PIM_COMMAND_REG", PIM_COMMAND_REG);
}

//*************************************
inline void execute_command()
{
	switch ( PIM_COMMAND_REG )
	{
		case PIM_COMMAND_NOP:			pim_error("Cannot execute NOP command!"); break;
		case PIM_COMMAND_DEMO:			execute_demo(); break;
		case PIM_COMMAND_RUN_KERNEL:	execute_kernel(); break;
		case PIM_COMMAND_HALT:			pim_error("Not implemented yet!"); break;
		default:						pim_error("Illegal command");
	}
}

//*************************************
bool new_command_received()
{
// 	pim_print_char("new_command_received: IM_COMMAND_REG", PIM_COMMAND_REG);
// 	pim_print_char("new_command_received: PIM_STATUS_REG", PIM_STATUS_REG);
	if ( PIM_COMMAND_REG != PIM_COMMAND_NOP )
	{
		pim_assert(PIM_STATUS_REG == PIM_STATUS_SLEEP, "PIM received a command while not in sleep mode!");
		PIM_STATUS_REG = PIM_STATUS_BUSY;
		execute_command();
        /* Indicate end of execution on PIM. This is similar to an interrupt to the host */
        PIM_M5_REG=PIM_TIME_STAMP+6;
		return true;
	}
	return false;
}

//*************************************
void command_done()
{
	pim_assert(PIM_STATUS_REG == PIM_STATUS_BUSY, "PIM received a command while not in sleep mode!");
	pim_assert(PIM_COMMAND_REG != PIM_COMMAND_NOP, "PIM should keep the previous command in this state!");
	
// 	pim_print_char("command_done: PIM_COMMAND_REG", PIM_COMMAND_REG);
// 	pim_print_char("command_done: PIM_STATUS_REG", PIM_STATUS_REG);
	
	// TODO: later change this to interrupt
	// First we must inform the host that this command has been finished
	PIM_STATUS_REG = PIM_STATUS_DONE;
	while ( PIM_COMMAND_REG != PIM_COMMAND_NOP )
	{
		pim_print_msg("[WAIT]");
		pim_wait(1000);
// 		pim_print_char("command_done:wait IM_COMMAND_REG", PIM_COMMAND_REG);
// 		pim_print_char("command_done:wait PIM_STATUS_REG", PIM_STATUS_REG);
	}
	// Now we can go to SLEEP state
	PIM_STATUS_REG = PIM_STATUS_SLEEP;
}

#endif // _PIM_DEVICE_UTILS_
