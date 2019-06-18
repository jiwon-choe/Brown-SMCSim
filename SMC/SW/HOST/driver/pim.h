#ifndef _PIM_DEVICE_DRIVER_
#define _PIM_DEVICE_DRIVER_
#include "_params.h"
#include "defs.h"
//#include <linux/types.h>

/*
Notice: PIM has the same architecture as the host processors
If host is AARCH64, then PIM is AARCH64
If host is AARCH, then PIM is AARCH
* This will remove the pointer passing limitations
*/

// PIM address and data types (32b for ARMv7 and 64b for ARMv8)
#define ulong_t   unsigned long

/*****************************************************/
// Definitions (Private, not to be exposed to the API)
// Number of contiguous minor numbers allocated for this module
#define PIM_MOD_DEV_NO		1

/*****************************************************/
// Messages and Debugging facilities

// ALERT
#define PIM_ALERT(fmt,args...)      {printk(KERN_ALERT "[PIM.Driver]: " fmt "\n", ## args);}

// WARNING
#define PIM_WARN(fmt,args...)       {printk(KERN_WARNING "[PIM.Driver]: " fmt "\n", ## args);}

// Only if debug mode:
#ifdef DEBUG_DRIVER

// NOTICE
#define PIM_NOTICE(fmt,args...)     {printk(KERN_NOTICE "[PIM.Driver]: " fmt "\n", ## args);}

// Hard Assertion
#define ASSERT(cond)                {if (!(cond)) {printk(KERN_ALERT "[PIM.Driver] Assertion failed in %s LINE: %d", __FILE__, __LINE__);}}

// If assertion does not hold, print message and return
#define ASSERT_RET(cond, msg, err)  {if (!(cond)) {printk(KERN_ALERT "[PIM.Driver] Assertion failed in %s LINE: %d MSG: %s", __FILE__, __LINE__, msg); return err;}}

// INFO
#define PIM_INFO(fmt,args...)		printk(KERN_INFO "[PIM.Driver]: " fmt "\n", ## args)

// Soft Assertion
#define ASSERT_DBG(cond)			{if (!(cond)){printk(KERN_ALERT "[PIM.Driver] Assertion failed in %s LINE: %d", __FILE__, __LINE__);}}

// Print the bits of this number
#define PRINT_BITS(X)				{int i; for(i=8*sizeof((X)-1; i>=0; i--) {((X) & (1 << i)) ? printk("1") : printk("0");}printk("\n");}

// Disable soft messages
#else

#define PIM_NOTICE(fmt,args...)     /* Do nothing */
#define ASSERT(cond)                /* Do nothing */
#define ASSERT_RET(cond, msg, err)  /* Do nothing */
#define PIM_INFO(fmt,args...)		/* Do nothing */
#define ASSERT_DBG(cond)			/* Do nothing */
#define PRINT_BITS(X)				/* Do nothing */

#endif

/*****************************************************/
// PIM device structure

typedef struct {
    dev_t dev;						// Device number
    struct file_operations *fops;	// File operations
    struct cdev cdev;				// Character device structure
    int min;						// Minor number
    int maj;						// Major number
    void *mem;						// memory mapped region (kernel virtual address)
} PIMDevice;

/*
List of pages pinned for use by the PIM device, along with their
physical address
*/
typedef struct {
    struct page** list;             // Pages pinned for use by the PIM device
    unsigned count;                 // Number of pages pinned
    ulong_t *physical_addr;      // Physical address of each page
} Pages;

/*
Slice Table is a structure on the DRAM (an array of slices), with
its pointer passed to PIM. PIM can use this structure to reprogram
its own TLB whenever a TLB miss occurs.
The difference between SliceTable and a PageTable is that slices
can have arbitrary sizes and can point to multiple contiguous pages
*/
typedef struct {
    ulong_t vaddr;               // Start address of the slice (Virtual Address)
    ulong_t paddr;               // Start address of the slice (Physical Address)
    ulong_t   size;                   // Size of the slice (multiple pages)
} Slice;

/*****************************************************/
// PIM file operations

// IOCTL
long pim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// OPEN
int pim_open(struct inode * inode, struct file * p_file);

// RELEASE
int pim_release(struct inode *p_inode, struct file * p_file);

// READ
ssize_t pim_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);

// WRITE
ssize_t pim_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);

// MMAP
int pim_mmap(struct file *filp, struct vm_area_struct *vma);

// LLSEEK
loff_t pim_llseek(struct file *filp, loff_t off, int whence);

/*****************************************************/
// Utilities

// Pin user pages into memory
int pim_get_user_pages(ulong_t addr_start, ulong_t n_pages);

// Create a slice table based on the pinned pages
int pim_create_slice_table(ulong_t vaddr_start);

// Write 1 byte to the memory mapped region of PIM
void pim_write_byte(ulong_t offset, char data);

// Read 1 byte from the memory mapped region of PIM
char pim_read_byte(ulong_t offset);

// Write a (ulong_t) to the memory mapped region of PIM (4Bytes for ARM7 and 8Bytes for ARM8)
void pim_write_ulong_t(ulong_t offset, ulong_t data);

// Read a (ulong_t) from the memory mapped region of PIM (4Bytes for ARM7 and 8Bytes for ARM8)
ulong_t pim_read_ulong_t(ulong_t offset);

// Flush the caches for all pages in the pages.list
int pim_cache_flush(void);

#endif // _PIM_DEVICE_DRIVER_
