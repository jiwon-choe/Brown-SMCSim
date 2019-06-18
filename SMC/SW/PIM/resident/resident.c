#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "registers.h"
#include "utils.h"

/*
 * Handshaking Mechanism
 * HOST		PIM
 * ---------------------
 * NOP		IDLE
 * START	
 * 		BUSY
 * 		DONE
 * NOP
 * 		IDLE
 */

int c_entry() {

	// Initialize device registers
	pim_print_msg("*********** initializing registers ...");
	initialize_registers();
    pim_print_msg("*********** initialization finished");

	// Main loop
	pim_print_msg("Going to sleep ...");
	while (1)
	{
		pim_print_msg("[SLEEP]");
		__asm__("wfi");
		pim_print_msg("[WAKE-UP]");
		if ( new_command_received() )
		{
			pim_print_msg("Finished execution of the kernel ...");
			command_done();
		}
		else
			pim_error("Woke up but no new command has arrived!");
        PIM_DMA_CLI = 0xFF;

	}
	pim_exit(PIM_ERROR_FAIL);

    __asm__("wfi");
	return 0;
}

// The computation kernel
// Note: Must be placed after the c_entry function, for dynamic offloading
#include "kernel.c"
