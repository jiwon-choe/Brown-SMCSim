#ifndef _PIM_DEVICE_REGISTERS_
#define _PIM_DEVICE_REGISTERS_

#include "definitions.h"
#include <stdint.h>
#include "_params.h"

/* The registers of the PIM device */
volatile uint8_t  PIM_STATUS_REG;       // The status of the PIM device
volatile uint8_t  PIM_COMMAND_REG;      // Command given from host to the PIM
volatile uint8_t  PIM_ERROR_REG;        // Error code is written here when an error happens
volatile uint8_t  PIM_INTR_REG;         // Interrupt register, writing to this location will wake-up the PIM device
volatile uint8_t  PIM_DEBUG_REG;        // Writing to this register will cause gem5 to display the contents in stdout or gem5.log

/* 
  The Slice Table:
  The actual slice table resides in DRAM, and these are only pointers
 */
volatile ulong_t PIM_SLICETABLE;            // Pointer to the slice table
volatile ulong_t PIM_SLICECOUNT;            // Number of slices present in the slice table
volatile ulong_t PIM_SLICEVSTART;           // Virtual address of the start of the memory region

/*
     -------------      <--- PIM_SLICETABLE (pointer)
    |SLICE_0_VADDR|     <--- PIM_SLICEVSTART
    |SLICE_0_PADDR|     <--- Slice 0
    |SLICE_0_SIZE |
     -------------
    |SLICE_1_VADDR|
    |SLICE_1_PADDR|     <--- Slice 1
    |SLICE_1_SIZE |
     -------------
    |...          |     <--- Slice N-1
     -------------  
*/

/* 
  Vector register-file of PIM. Can be used for data passing from host, for
  internal use, or for DMA transfers.
 */
volatile uint8_t  PIM_VREG[PIM_VREG_SIZE];         // Vector Register File (bytes)

/* 
  Scalar registers of PIM. Can be used for pointer or data
  passing from host.
 */
volatile ulong_t PIM_SREG[PIM_SREG_COUNT];         // Scalar Registers (words)

/* PIM's general purpose registers:
     -------------
    | PIM_SREG[0] |     <--- Scalar Registers
    | PIM_SREG[1] |
    | ...         |
     -------------
    |             |
    |   PIM_VREG  |     <--- Vector Register File
    |             |
     -------------
*/

/*
  Registers for communication with gem5 (Debugging and hacks)
*/
volatile uint8_t PIM_M5_REG;           // Writing to this register will give commands to the gem5 simulator (for debugging purposes)
volatile ulong_t PIM_M5_D1_REG;        // 1-word data
volatile ulong_t PIM_M5_D2_REG;        // 1-word data

/*
  Accessing the DTLB prefetching engine inside the PIM device
  PIM_TLB_IDEAL_PREFETCH:
  Writing a virtual address to this register will cause
  a functional of that virtual address. So immediately after
  this line, the rule will be present in the DTLB
*/
volatile ulong_t PIM_DTLB_IDEAL_REFILL;

/* 
  DMA Access registers:
  PIM's DMA can transfer data between PIM's vector register file and a memory location:
          SPM               Main Memory
     -------------         -------------        
    |  PIM_VREG   | <---> | MEM_ADDRESS |
     -------------         -------------
 */
volatile ulong_t PIM_DMA_MEM_ADDR;          // (Virtual Address) of the main memory location to transfer to/from
volatile ulong_t PIM_DMA_SPM_ADDR;          // (Physical Address) of the SPM location to transfer to/from (should preferably point to VREG)
volatile ulong_t PIM_DMA_NUMBYTES;          // Number of bytes to transfer
volatile uint8_t PIM_DMA_COMMAND;           // PIM DMA Command Register
volatile uint8_t PIM_DMA_STATUS;            // Status of the DMA Resources
volatile uint8_t PIM_DMA_CLI;               // Clear Interrupt Request

/*
Sending atomic commands to the HMC.
Instead of modifying PIM's ISA, we have devised these registers.
Writing the intended virtual address to these registers will do the intended HMC command
*/

volatile ulong_t HMC_ATOMIC_INCR;            // Atomic Integer Increment by 1
volatile ulong_t HMC_ATOMIC_IMIN;            // Atomic Integer Min
volatile ulong_t HMC_ATOMIC_FADD;            // Atomic Float Add by HMC_OPERAND
volatile ulong_t HMC_OPERAND;                // Operand for HMC commands

/*
Arithmetic Coprocessor of PIM (Vector Processor)
This coprocessor can work on PIM's scalar and vector registers and put back the
generated result in them.
*/

volatile uint8_t PIM_COPROCESSOR_CMD;        // Command to the PIM Coprocessor

#endif // _PIM_DEVICE_REGISTERS_