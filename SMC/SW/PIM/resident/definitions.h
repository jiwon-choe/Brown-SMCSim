#ifndef _PIM_DEVICE_DEFINITIONS_
#define _PIM_DEVICE_DEFINITIONS_

#define ulong_t   unsigned long       // 32b on ARMv8 and 64b on ARMv8

/*
  These are the fixed definitions of the PIM device 
  Notice: that changing this file requires rebuilding 
  gem5 again.
  Notice: a link to this file is automatically added to gem5
*/

// PIM_COMMAND_REG values
#define PIM_COMMAND_NOP         'N' // No operation
#define PIM_COMMAND_HALT        'H' // Halt execution
#define PIM_COMMAND_DEMO        'D' // Demo: VREG = INCR(VREG)
#define PIM_COMMAND_RUN_KERNEL  'K' // Run the offloaded kernel

// PIM_STATUS_REG values
#define PIM_STATUS_SLEEP        'S' // Sleep
#define PIM_STATUS_BUSY         'B' // Busy executing command
#define PIM_STATUS_DONE         'D' // Done executing command

// PIM_ERROR_REG values
#define PIM_ERROR_NONE          0x00    // No error
#define PIM_ERROR_FAIL          0x01    // General failure

// PIM_INTR_REG values
#define PIM_INTR_TRIGGER        0x01    // Trigger the interrupt
#define PIM_INTR_NOP            0x00    // Do nothing

// PIM_DTLB_FLAGS values
#define PIM_TLB_EN              0x01    // This rule is enabled or not
#define PIM_TLB_REN             0x02    // Read Enable Flag
#define PIM_TLB_WEN             0x04    // Write Enable Flag

// PIM_DMA_COMMAND values
#define PIM_DMA_READ            0b00000000  // Transfer from Memory to PIM's SPM
#define PIM_DMA_WRITE           0b10000000  // Transfer from PIM's SPM to Memory

// DMA Resources ( Bitwise Or with PIM_DMA_COMMAND )
#define DMA_RES0                0b00000001
#define DMA_RES1                0b00000010
#define DMA_RES2                0b00000100
#define DMA_RES3                0b00001000
#define DMA_RES4                0b00010000
#define DMA_RES5                0b00100000
#define DMA_RES6                0b01000000

// Commands to gem5 (for debugging purpose)
#define PIM_ENABLE_PACKET_LOG       'L'
#define PIM_DISABLE_PACKET_LOG      'D'
#define PIM_TIME_STAMP              '0' // Record timestamp in gem5, also dump statistics
#define PIM_TIME_STAMP_MAX          '9' // Maximum time stamp: '0', '1', ..., '9'
#define PIM_HOST_CACHE_FLUSH_HACK   'F' // This is a simulation hack to implement the cache flush operation
#define PIM_EXIT_GEM5               'X'
#define PIM_M5_IGNORE               'I' // This is a NOP command and does nothing (used for initialization)

// HMC Atomic Commands
#define HMC_CMD_NOP                  0 // Nop
#define HMC_CMD_INC                 '1' // Integer MEM = MEM + 1
#define HMC_CMD_FADD                '2' // Float   MEM = MEM + HMC_OPERAND
#define HMC_CMD_MIN                 '3' // Integer MEM = MIN( MEM, HMC_OPERAND )

// PIM_COPROCESSOR_CMD values
#define CMD_NOP                      0   // Nop
#define CMD_TO_FLOAT                 1 // PIM_SREG[1] = float(PIM_SREG[0])                  // Convert to Float
#define CMD_FDIV                     2 // PIM_SREG[2] = PIM_SREG[0] / PIM_SREG[1]           // Divide
#define CMD_FMUL                     3 // PIM_SREG[2] = PIM_SREG[0] * PIM_SREG[1]           // Multiply
#define CMD_FCMP                     4 // PIM_SREG[2] = (PIM_SREG[0]>PIM_SREG[1])?1.0:0     // Compare
#define CMD_FACC                     5 // PIM_SREG[1] = PIM_SREG[0] + PIM_SREG[1]           // Accumulate
#define CMD_FSUB                     6 // PIM_SREG[2] = PIM_SREG[0] - PIM_SREG[1]           // Subtract
#define CMD_FABS                     7 // PIM_SREG[1] = fabsf(PIM_SREG[0])                  // Absolute Value
#define CMD_FPRINT                   8 // print(PIM_SREG[0])                                // Print to the screen
#define CMD_TO_FIXED                 9 // PIM_SREG[1] = int(PIM_SREG[0]*1000000.0)          // Convert to Fixed-point
#define CMD_CUSTOM_PR1              10 // Custom instruction sequence for PageRank

#endif // _PIM_DEVICE_DEFINITIONS_