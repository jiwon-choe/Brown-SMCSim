#ifndef _PIM_DEVICE_DRIVER_DEFS_
#define _PIM_DEVICE_DRIVER_DEFS_

/*****************************************************/
// Constant Definitions (To be exposed to the API)

// IOCTL Commands
#define PIM_IOCTL_MAGIC 			'p'
#define PIM_IOCTL_HELLO				 _IO(PIM_IOCTL_MAGIC,0xB0)
#define PIM_IOCTL_ALLOCATE_DATA     _IOW(PIM_IOCTL_MAGIC,0xB1,unsigned)
#define PIM_IOCTL_RELEASE_ALL_DATA   _IO(PIM_IOCTL_MAGIC,0xB2)
#define PIM_IOCTL_REPORT_STATS       _IO(PIM_IOCTL_MAGIC,0xB3)

// Maximum number of (unsigned long) arguments to pass to ioctl
// Notice: the first argument is the number of arguments being passed
#define PIM_IOCTL_MAX_ARGS 			8

/*****************************************************/
// Bit Manipulation Functions
// 1[h, l]		e.g. 0011110
#define GET_BIT_MASK(h, l)      ( (BIT((h)-(l)+1)-1) << (l) )

// X[h, l]	e.g. 0011010
#define GET_BIT_RANGE(X, h, l)  ( (GET_BIT_MASK((h), (l)) & (X)) >> (l) )

// Calculate the order of the pages required
#define PAGE_ORDER(X)           ((X<=0x1000)?0:(X<=0x2000)?1:(X<=0x4000)?2:(X<=0x8000)?3:(X<=0x10000)?4:(X<=0x20000)?5:(X<=0x40000)?6:11);


#endif // _PIM_DEVICE_DRIVER_DEFS_
