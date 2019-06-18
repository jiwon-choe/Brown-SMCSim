#!/bin/bash
# Link script
echo -e "ENTRY(_start)
SECTIONS
{
	/******************************************* HEADER *******************************************/
	. = 0x00000010;				/* This code will be remapped to the PIM_ADDRESS_BASE */
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
    /******************************************** PLT SEGMENT *************************************/
    . = $PIM_PLT_OFFSET;
	.plt            : { *(.plt) }
	
	/******************************************* CODE SEGMENT *************************************/
	. = $PIM_TEXT_OFFSET;
	.text           :
	{
	boot.o
	. = ALIGN(8);
	*(.text)
	}
	.fini           : { KEEP (*(.fini)) }
	PROVIDE(__etext = .);
	PROVIDE(_etext = .);
	PROVIDE(etext = .);				/* Provide the end of the text section */
	
	/******************************************* READONLY DATA ************************************/
	. = $PIM_RODATA_OFFSET;
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
	. = $PIM_ARRAY_OFFSET;
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
	. = $PIM_GOT_OFFSET;
	.got            :
	{ 
	*(.got.plt)
	*(.got)
	. = ALIGN(8);
	}
	
	/******************************************* DATA SECTION *************************************/
	. = $PIM_DATA_OFFSET;
	.data           :
	{
	__data_start = .;
	*(.data .data.*)
	. = ALIGN(8);
	}
	_edata = .;
	PROVIDE(edata = .);
	
	/******************************************* BSS SECTION **************************************/
	. = $PIM_BSS_OFFSET;
	
	__bss_start = .;
	__bss_start__ = .;
	.bss            :
	{
	*(.bss .bss.*)
	. = ALIGN(8);
	}
	__bss_end__ = .;
	_bss_end__ = .;
	bss_size = __bss_end__ - __bss_start__;  
	. = ALIGN(8);
	
	/******************************************* STACK SECTION*************************************/
	. = $PIM_STACK_OFFSET;	/* Notice: stack is shared with BSS and grows towards 0x0 address */
	stack_top = .;		/* Must be aligned */
	__end = .;
	_end = .;
	PROVIDE(end = .);
}
"

