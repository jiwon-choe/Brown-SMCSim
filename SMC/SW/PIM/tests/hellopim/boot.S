/*************************************************************************
 * Super simple bootloader
 * Preserve loaded values that we need to pass to the kernel (r0, r1, r2)
 * Additionally M5 puts the kernel start address in r3
 *
 * Upon executing this code:
 * r0 = 0, r1 = machine number, r2 = atags ptr
 * r3 = kernel start address, r4 = GIC address, r5 = flag register address
 *
 * CPU 0 should branch to the kernel start address and it's done with
 * the boot loader. Other CPUs need to start in a wfi loop. When CPU0 sends
 * an IPI the slave CPUs reads a register which CPU0 has programmed with the
 * boot address for the secondary cpu
 **************************************************************************/
.text
.globl  _start
.extern	main
_start:
_entry:
    b bootldr // All the interrupt vectors jump to the boot loader
    b bootldr
    b bootldr
    b bootldr
    b bootldr
    b bootldr
    b bootldr
    b bootldr
    b bootldr

bootldr:
    SUB r0, r0, r0		// Erfan: Set R0 to Zero
    LDR r3, =c_entry		// Erfan: Load r3 with start address
    LDR sp, =stack_top		// Erfan: Stack top
    mrc p15, 0, r8, c0, c0, 5 // get the MPIDR register
    bics r8, r8, #0xff000000  // isolate the lower 24 bits (affinity levels)
    bxeq r3                   // if it's 0 (CPU 0), branch to kernel (Erfan: this will branch to c_entry)
    mov  r8, #1
    str  r8, [r4, #0]         //  Enable CPU interface on GIC
    wfi                       //  wait for an interrupt
pen:
    ldr r8, [r5]              // load the value
    movs r8, r8               // set the flags on this value
    beq pen                   // if it's zero try again
    bx r8                     // Jump to where we've been told
    bkpt                      // We should never get here
