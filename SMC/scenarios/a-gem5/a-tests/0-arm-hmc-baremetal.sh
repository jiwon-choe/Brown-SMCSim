#!/bin/bash
if ! [ -z $1 ] && ( [ $1 == "7" ] || [ $1 == "8" ] ); then echo "Arch: ARM$1"; else echo "First argument must be architecture: 7 or 8 (ARM7 or ARM8)"; exit; fi

source UTILS/default_params.sh
create_scenario "$0/$*" "default" "ARMv$1 + HMC2011 + Linux (VExpress_EMM)"
####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm$1.sh

print_msg "To see what is going on: export GEM5_DEBUGFLAGS=\"Fetch,Decode,ExecAll\""
print_msg "We must wait at the WFI instruction\""

export GEM5_NUMCPU=1
export GEM5_DEBUGFLAGS=Fetch,Decode,ExecAll,Registers
export GEM5_BARE_METAL_HOST=TRUE

source ./smc.sh -u $*	# Update these variables in the simulation environment

# Link script (same for ARMv7 and ARMv8)
# Link script
######################################################################################################
echo -e "
ENTRY(_start)
SECTIONS
{
	/******************************************* HEADER *******************************************/
	. = 0x80000010;				/* This code will be remapped to the PIM_ADDRESS_BASE */
	.interp         : { *(.interp) }
	.note.ABI-tag   : { *(.note.ABI-tag) }
	.hash           : { *(.hash) }
	.dynsym         : { *(.dynsym) }
	.dynstr         : { *(.dynstr) }
	.version        : { *(.version) }
	.version_d      : { *(.version_d) }
	.version_r      : { *(.version_r) }
	.gnu.version_r  : { *(.gnu.version_r) }
	.gnu.version    : { *(.gnu.version) }
	.rel.dyn        : { *(.rel.dyn) }
	.rela.dyn       : { *(.rela.dyn) }
	.rel.plt        : { *(.rel.plt) }
	.rela.plt       : { *(.rela.plt) }
	.init           : { KEEP (*(.init)) }
	.plt            : { *(.plt) }
	
	/******************************************* CODE SEGMENT *************************************/
	. = 0x80010000;
	.text           :
	{
	boot.o
	*(.text)
	}
	.fini           : { KEEP (*(.fini)) }
	PROVIDE(__etext = .);
	PROVIDE(_etext = .);
	PROVIDE(etext = .);				/* Provide the end of the text section */
	
	/******************************************* READONLY DATA ************************************/
	. = 0x80020000;
	.rodata         : { *(.rodata .rodata.*) }
	
	/* ARM Exception Handling - Commented */
	/*__exidx_start = .;				
	.ARM.exidx   : { *(.ARM.exidx*) }
	__exidx_end = .;
	*/
	
	. = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) & (CONSTANT (MAXPAGESIZE) - 1));
	. = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));

	/* Thread private data - Commented */
	/*.tdata	  : { *(.tdata .tdata.*) }
	.tbss		  : { *(.tbss .tbss.*) }
	*/
	
	/******************************************* ARRAY INIT AND FINISH ****************************/
	. = 0x80030000;
	.preinit_array     :
	{
	PROVIDE_HIDDEN (__preinit_array_start = .); 
	KEEP (*(.preinit_array))
	PROVIDE_HIDDEN (__preinit_array_end = .); 
	}
	.init_array     :
	{
	PROVIDE_HIDDEN (__init_array_start = .); 
	KEEP (*(.init_array*))
	PROVIDE_HIDDEN (__init_array_end = .); 
	}
	.fini_array     :
	{
	PROVIDE_HIDDEN (__fini_array_start = .); 
	KEEP (*(.fini_array*))
	PROVIDE_HIDDEN (__fini_array_end = .); 
	}
	
	/* Dynamic Section - Commented */
	/*.dynamic        : { *(.dynamic) }*/
	
	/******************************************* GOT SECTION **************************************/
	. = 0x80040000;
	.got            :
	{ 
	*(.got.plt)
	*(.got)
	. = ALIGN(4);
	}
	
	/******************************************* DATA SECTION *************************************/
	. = 0x80050000;
	.data           :
	{
	__data_start = .;
	*(.data .data.*)
	. = ALIGN(4);
	}
	_edata = .;
	PROVIDE(edata = .);
	
	/******************************************* BSS SECTION **************************************/
	. = 0x80060000;
	
	__bss_start = .;
	__bss_start__ = .;
	.bss            :
	{
	*(.bss .bss.*)
	. = ALIGN(. != 0 ? 32 / 8 : 1);
	}
	__bss_end__ = .;
	__bss_end = .;
	_bss_end__ = .;
	/* Erfan removed this: bss_size = __bss_end__ - __bss_start__; */
	bss_size = 0x0000FFFF;
	. = ALIGN(4);
	
	/******************************************* STACK SECTION*************************************/
	. = 0x80070000;
	stack_top = .;		/* Must be aligned */
	__end = .;
	_end = .;
	PROVIDE(end = .);
}
" > $SCENARIO_CASE_DIR/resident.ld

######################################################################################################
######################################################################################################
######################################################################################################
####################################### ARMv7 ########################################################
######################################################################################################
######################################################################################################
######################################################################################################
if [ $1 == "7" ]; then

_PWD=${PWD}
cd $SCENARIO_CASE_DIR

# The executable code (bare-metal)
######################################################################################################
echo -e "
int x;
int c_entry()
{
	int a = 2;
	x = a;
	__asm__(\"wfi\");
	return x;
}
" > $SCENARIO_CASE_DIR/resident.c

# Boot loader
######################################################################################################
echo "
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

    // Enable Floating Point Unit (FPU)
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #0x300000            /* single precision */
    orr r0, r0, #0xC00000            /* double precision */
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0x40000000
    fmxr fpexc,r0

    SUB r0, r0, r0		// Erfan: Set R0 to Zero
    LDR r3, =c_entry		// Erfan: Load r3 with start address
    LDR sp, =stack_top		// Erfan: Stack top
    mrc p15, 0, r8, c0, c0, 5 // get the MPIDR register
    bics r8, r8, #0xff000000  // isolate the lower 24 bits (affinity levels)
    bxeq r3                   // if it's 0 (CPU 0), branch to kernel
    mov  r8, #1
    str  r8, [r4, #0]         //  Enable CPU interface on GIC
    wfi                       //  wait for an interrupt
pen:
    ldr r8, [r5]              // load the value
    movs r8, r8               // set the flags on this value
    beq pen                   // if it's zero try again
    bx r8                     // Jump to where we've been told
    bkpt                      // We should never get here
" > $SCENARIO_CASE_DIR/boot_arm7.S

# Do the linking
######################################################################################################
echo -e "
	print_msg \" > Boot loader ...\"
	${HOST_CROSS_COMPILE}gcc -mfloat-abi=softfp -march=armv7-a -fno-builtin -nostdinc -o boot.o -c boot_arm7.S -Wall -O0
	print_msg \" > Resident code ...\"
	${HOST_CROSS_COMPILE}gcc -c -march=armv7-a -g resident.c -o resident.o -mno-thumb-interwork -fno-stack-protector -fPIC -Wall -O0
	${HOST_CROSS_COMPILE}gcc -c -march=armv7-a -g resident.c -o resident.s -mno-thumb-interwork -fno-stack-protector -fPIC -S -Wall -O0
	print_msg \" > Link ...\"
	${HOST_CROSS_COMPILE}ld -T resident.ld resident.o boot.o -lc -lgcc -lgcc_eh -o resident.elf -non_shared -static -O0 \
		--library-path /usr/arm-linux-gnueabihf/lib/ \
		--library-path /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/
" > $SCENARIO_CASE_DIR/build.sh
chmod +x build.sh
source build.sh
cd $_PWD

export GEM5_KERNEL=$SCENARIO_CASE_DIR/resident.elf
	
else

######################################################################################################
######################################################################################################
######################################################################################################
####################################### ARMv8 ########################################################
######################################################################################################
######################################################################################################
######################################################################################################

_PWD=${PWD}
cd $SCENARIO_CASE_DIR

# The executable code (bare-metal)
######################################################################################################
echo -e "
int x;
int c_entry()
{
    float f1, f2, f3;
    f3 = f2*f1;
	int a = 2;
	x = a + (int)f3;
	
	__asm__(\"wfi\");
	return x;
}
" > $SCENARIO_CASE_DIR/resident.c

# Boot loader
######################################################################################################
echo "
.text
.globl	_start
_start:
        /*
         * EL3 initialisation
         */
		ldr x7, =start_label
		br x7
start_label:         
        mrs	x0, CurrentEL
        cmp	x0, #0xc			// EL3?
        b.ne	start_ns			// skip EL3 initialisation

        mov	x0, #0x30			// RES1
        orr	x0, x0, #(1 << 0)		// Non-secure EL1
        orr	x0, x0, #(1 << 8)		// HVC enable
        orr	x0, x0, #(1 << 10)		// 64-bit EL2
        msr	scr_el3, x0

        msr	cptr_el3, xzr			// Disable copro. traps to EL3

        ldr	x0, =CNTFRQ
        msr	cntfrq_el0, x0

        /*
         * Check for the primary CPU to avoid a race on the distributor
         * registers.
         */
        mrs	x0, mpidr_el1
        tst	x0, #15
        b.ne	1f				// secondary CPU

        ldr	x1, =GIC_DIST_BASE		// GICD_CTLR
        mov	w0, #3				// EnableGrp0 | EnableGrp1
        str	w0, [x1]

1:	ldr	x1, =GIC_DIST_BASE + 0x80	// GICD_IGROUPR
        mov	w0, #~0				// Grp1 interrupts
        str	w0, [x1], #4
        b.ne	2f				// Only local interrupts for secondary CPUs
        str	w0, [x1], #4
        str	w0, [x1], #4

2:	ldr	x1, =GIC_CPU_BASE		// GICC_CTLR
        ldr	w0, [x1]
        mov	w0, #3				// EnableGrp0 | EnableGrp1
        str	w0, [x1]

        mov	w0, #1 << 7			// allow NS access to GICC_PMR
        str	w0, [x1, #4]			// GICC_PMR

        msr	sctlr_el2, xzr

        /*
         * Prepare the switch to the EL2_SP1 mode from EL3
         */
        ldr	x0, =start_ns			// Return after mode switch
        mov	x1, #0x3c9			// EL2_SP1 | D | A | I | F
        msr	elr_el3, x0
        msr	spsr_el3, x1
        eret

start_ns:
        /*
         * Kernel parameters
         */
        mov	x0, xzr
        mov	x1, xzr
        mov	x2, xzr
        mov	x3, xzr

        mrs	x4, mpidr_el1
        tst	x4, #15
        b.eq	2f

        /*
         * Secondary CPUs
         */
1:	wfe
        ldr	x4, =PHYS_OFFSET + 0xfff8
        ldr     x4, [x4]
        cbz	x4, 1b
        br	x4				// branch to the given address

2:
        /*
         * UART initialisation (38400 8N1)
         */
/*      ldr	x4, =UART_BASE			// UART base
        mov	w5, #0x10			// ibrd
        str	w5, [x4, #0x24]
        mov	w5, #0xc300
        orr	w5, w5, #0x0001			// cr
        str	w5, [x4, #0x30]
*/

        /*
         * CLCD output site MB
         */
/*      ldr	x4, =SYSREGS_BASE
        ldr	w5, =(1 << 31) | (1 << 30) | (7 << 20) | (0 << 16)	// START|WRITE|MUXFPGA|SITE_MB
        str	wzr, [x4, #0xa0]		// V2M_SYS_CFGDATA
        str	w5, [x4, #0xa4]			// V2M_SYS_CFGCTRL
*/

        // set up the arch timer frequency
        //ldr	x0, =CNTFRQ
        //msr	cntfrq_el0, x0

        /*
         * Primary CPU
         */
//        ldr	x0, =PHYS_OFFSET + 0x8000000	 // device tree blob
//        ldr     x6, =PHYS_OFFSET + 0x80000       // kernel start address
//        br	x6

        ldr x6, =stack_top
        mov sp, x6

        ldr x6, =c_entry
        br x6



        .ltorg

        .org	0x200

" > $SCENARIO_CASE_DIR/boot_arm8.S

# Do the linking
######################################################################################################
echo -e "
	print_msg \" > Boot loader ...\"
	${HOST_CROSS_COMPILE}gcc -march=armv8-a -fno-builtin -nostdinc -o boot.o -c boot_arm8.S -Wall -O0 -DPHYS_OFFSET=0x80000000 -DCNTFRQ=0x01800000 -DUART_BASE=0x1c090000 -DSYSREGS_BASE=0x1c010000 -DGIC_DIST_BASE=0x2c001000 -DGIC_CPU_BASE=0x2c002000 -mcmodel=large -mstrict-align -mtune=cortex-a57
	
	#Notice: -fPIC does not work in ARMv8
	
	print_msg \" > Resident code ...\"
	${HOST_CROSS_COMPILE}gcc -c -march=armv8-a -g resident.c -o resident.o -fno-stack-protector -Wall -O0
	${HOST_CROSS_COMPILE}gcc -c -march=armv8-a -g resident.c -o resident.s -fno-stack-protector -S -Wall -O0
	
	print_msg \" > Link ...\"
	${HOST_CROSS_COMPILE}ld -T resident.ld resident.o boot.o -lc -lgcc -lgcc_eh -o resident.elf -non_shared -static -O0 \
		--library-path /usr/aarch64-linux-gnu/lib \
		--library-path /usr/lib/gcc-cross/aarch64-linux-gnu/4.8
		
	# Disassembly output (for debugging purposes)
	${HOST_CROSS_COMPILE}objdump -S --disassemble resident.elf > resident.elf.disasm
		
" > $SCENARIO_CASE_DIR/build.sh

chmod +x build.sh
source build.sh
cd $_PWD

export GEM5_KERNEL=$SCENARIO_CASE_DIR/resident.elf

fi

####################################
source ./smc.sh $*
print_msg "Done!"

