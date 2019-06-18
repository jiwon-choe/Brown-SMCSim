
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_ALERT */
#include <linux/kdev_t.h>	/* MAJOR */
#include <linux/interrupt.h>
#include <asm/io.h>			/* ioremap, iounmap, iowrite32 */
#include <linux/cdev.h>		/* cdev struct */
#include <linux/fs.h>		/* file_operations struct */
#include <asm/uaccess.h>	/* copy_to_user, copy_from_user, access_ok */
#include <linux/mm.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/slab.h>		/* kmalloc */
//#include <asm/cacheflush.h>     /* __cpuc_flush_dcache_area, outer_cache.flush_range */
//#include <asm/outercache.h>
#include <linux/highmem.h>      /* kmap, kunmap */
#include <linux/pagemap.h>      /* page_cache_release() */

#include "pim.h"
#include "_params.h"
#include "definitions.h"	// resident/definitions.h

/*****************************************************/
/* Performance Counters and statistics */
unsigned stat_numslices;  // Number of slices in the slice table

/*****************************************************/
// File operations
struct file_operations pim_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = pim_ioctl,
	.open = pim_open,
	.release = pim_release,
	.llseek = pim_llseek,
	.read = pim_read,
	.write = pim_write,
	.mmap = pim_mmap };

// Static variables
static PIMDevice pimdevice;				// The PIM Device struct
static Pages pages;						// List of the pinned pages
static struct class *pimclass;			// Class of our device
static Slice* slicetable;       		// Slice Table
static ulong_t slicetable_paddr;			// Physical address of the slice-table
static ulong_t max_user_va;				// Maximum VA used by user (Used for overlap check in AARCH64 systems)

// Recurrency checks (For now, only one user can use the driver)
static int pim_init_count = 0;
static int pim_ioctl_allocate = 0;

// VM_RESERVERD for mmap
#ifndef VM_RESERVED
# define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

/*****************************************************/
// Init the module
static int __init pim_init(void)
{
	int err, devno;
	PIM_INFO("pim_init Loading device driver for PIM!");
	pimdevice.min = 0;
    pim_init_count++;
    ASSERT(pim_init_count == 1); // For now, only one user can use the driver
	pages.list = NULL;
	pages.physical_addr = NULL;
	pages.count = 0;
	max_user_va = 0;
    /* Reset statistics */
    stat_numslices = 0;
	/* Checking integer size of the system. Because we must work both with ARMv7 and ARMv8 */
	PIM_INFO("sizeof(int)=%ld", (unsigned long)sizeof(int));
	PIM_INFO("sizeof(int*)=%ld", (unsigned long)sizeof(int*));
	PIM_INFO("sizeof(ulong_t)=%ld", (unsigned long)sizeof(ulong_t));
    ASSERT(sizeof(ulong_t)==pim_arch/8);
    ASSERT(sizeof(ulong_t)==sizeof(void*));
    ASSERT(sizeof(unsigned)==4); // Used as ioctl argument size

	// INIT MODULE CHAR DEVICE
	err = alloc_chrdev_region(&pimdevice.dev, pimdevice.min, PIM_MOD_DEV_NO, "PIM");

	if ((pimclass = class_create(THIS_MODULE, "chardrv")) == NULL) {
		unregister_chrdev_region(pimdevice.dev, 1);
		PIM_WARN("PIM error creating class");
		return -1;
	}
	if (device_create(pimclass, NULL, pimdevice.dev, NULL, "PIM") == NULL) {
		class_destroy(pimclass);
		unregister_chrdev_region(pimdevice.dev, 1);
		PIM_WARN("PIM error creating device");
		return -1;
	}

	pimdevice.maj = MAJOR(pimdevice.dev);
	if (err < 0)
	{
		PIM_WARN("PIM can't get the major %d", pimdevice.maj);
	}

	pimdevice.fops = &pim_fops;

	// STRUCT CDEV
	devno = MKDEV(pimdevice.maj, pimdevice.min);
	cdev_init(&pimdevice.cdev, pimdevice.fops);
	pimdevice.cdev.owner = THIS_MODULE;
	pimdevice.cdev.ops = pimdevice.fops;
	err = cdev_add(&pimdevice.cdev, devno, PIM_MOD_DEV_NO);
	PIM_INFO("cdev added, major number: %d, minor number: %d", pimdevice.maj, pimdevice.min);
	if (err!=0) {
		PIM_NOTICE("Error opening the device: %d\n", err);
		return err;
	}
	
	//remapping the physical addresses range to the kernel virtual memory map
	pimdevice.mem = ioremap_nocache(PHY_BASE, PHY_SIZE);
	PIM_INFO("PIM physical range [%lx:%lx] mapped to kernel space @ %lx",(ulong_t)PHY_BASE, (ulong_t)(PHY_BASE+PHY_SIZE-1), (unsigned long)pimdevice.mem);

	return 0;
}

/*****************************************************/
// Unload and exit the module
static void __exit pim_exit(void)
{
	PIM_INFO("pim_exit");
	cdev_del(&pimdevice.cdev);
	unregister_chrdev_region(pimdevice.dev, PIM_MOD_DEV_NO);
	PIM_ALERT("Unloading device driver for PIM");
	return;
}

/*****************************************************/
// Implementation of the file operations

/*****************************************************/
// IOCTL
long pim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* local variabes */
	int i = 0;
	int err = 0;
	long retval = 1; // success
	unsigned num_ranges = 0;
	ulong_t ioctl_arg[PIM_IOCTL_MAX_ARGS];
	ulong_t ioctl_arg_size = 0;
	PIM_INFO("pim_ioctl");
	/* extract the type and number bitfields, and don't decode wrong cmds: return ENOTTY before
	   access_ok() */
	if (_IOC_TYPE(cmd) != PIM_IOCTL_MAGIC)
	{
		PIM_ALERT("Illegal command type in received in ioctl");
		return -ENOTTY;
	}
	if ( (_IOC_NR(cmd) < 0xB0) | (_IOC_NR(cmd) > 0xB3) )
	{
		PIM_ALERT("Illegal command number received in ioctl");
		return -ENOTTY;
	}

	/* The direction is a bitmask, and VERIFY_WRITE catches R/W transfers. 'Type' is user-oriented,
	   while access_ok is kernel-oriented, so the concept of "read" and "write" is reversed
	*/
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	{
		PIM_ALERT("Illegal access in ioctl");
		return -EFAULT;
	}

	// the actual ioctls
	switch(cmd) {
	//*******************
	case PIM_IOCTL_HELLO: // Only for debugging
		PIM_INFO("ioctl: hello");
	break;
	//*******************
	case PIM_IOCTL_ALLOCATE_DATA:	// Offload data to the PIM device

		PIM_INFO("ioctl: allocate data");
        pim_ioctl_allocate++;
        ASSERT(pim_ioctl_allocate == 1); // For now, this functionality works only once
		/* Copy the first byte only, this bytes indicates the number of bytes remaining to get from user space */
		if ( copy_from_user(ioctl_arg, (void __user *)arg, sizeof(ulong_t)) )
			return -EFAULT;
		ioctl_arg_size = ioctl_arg[0];
		if (ioctl_arg_size+1 > PIM_IOCTL_MAX_ARGS)
		{
			PIM_ALERT("ioctl: ioctl_arg_size+1 > PIM_IOCTL_MAX_ARGS");
			return -EFAULT;
		}
		PIM_INFO("ioctl: num arguments: %ld", (ulong_t) ioctl_arg_size );

		/* Now copy the whole ioctl_args based on ioctl_arg_size */
		if ( copy_from_user(ioctl_arg, (void __user *)arg, (ioctl_arg_size+1)* sizeof(ulong_t)) )
		{
			PIM_ALERT("ioctl: failed to copy the whole data");
			return -EFAULT;
		}

		num_ranges = ioctl_arg_size/2;
		ASSERT(pages.count == 0);
		ASSERT(num_ranges == 1);	// For now, we must have one contiguous range in the user

		/* Get user pages */
		for ( i=0; i< num_ranges; i++)
		{
			PIM_INFO("ioctl: range start:%lx, count:%ld", ioctl_arg[2*i+1], ioctl_arg[2*i+2] );
			pim_get_user_pages(ioctl_arg[2*i+1], ioctl_arg[2*i+2]); // TODO: Later: check protection flags of TLB
		}

		/* 
		  Create a page table and send its pointer to PIM.
		  Also, flush the caches to make sure the most recent data
		  is accessible by PIM
		 */
		pim_create_slice_table(ioctl_arg[1]);
	break;
	//*******************
	case PIM_IOCTL_RELEASE_ALL_DATA:	// Release all the pages
		PIM_INFO("ioctl: release all data");
		for (i=0; i<pages.count; i++) 
		{
			PIM_INFO("RELEASE PAGE[%d]: PHYSICAL ADDR: 0x%lx  PAGE STRUCT: 0x%lx", i, (ulong_t)pages.physical_addr[i], (ulong_t)(pages.list[i]) );
			if (!PageReserved(pages.list[i])) 
				SetPageDirty(pages.list[i]);
			else
				PIM_WARN("Page was reserved!");
			page_cache_release(pages.list[i]); 
			kfree(pages.list);
		}
	break;
    //*******************
    case PIM_IOCTL_REPORT_STATS:   // Report gathered statistics to the screen
    printk("****************** DRIVER STATISTICS ******************\n");
    printk("softwarestat.pages.count\t%d\n", pages.count);
    printk("softwarestat.slices.count\t%d\n", stat_numslices);
    printk("softwarestat.slices.reduction\t%d\n", pages.count - stat_numslices);
    break;
	//*******************
	default:
		return -ENOTTY;
	}

	return retval;
}

/*****************************************************/
// READ
ssize_t pim_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	PIM_INFO("pim_read");
	return -EINVAL;
}

/*****************************************************/
// WRITE
ssize_t pim_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	PIM_INFO("pim_write");
	return -EINVAL;  
}

/*****************************************************/
// LLSEEK
loff_t pim_llseek(struct file *filp, loff_t off, int whence)
{
	PIM_INFO("pim_llseek");
	return -EINVAL;
}

/*****************************************************/
// OPEN
int pim_open(struct inode * inode, struct file * p_file)
{
	PIMDevice *dev;
	PIM_INFO("pim_open");
	dev = container_of(inode->i_cdev, PIMDevice, cdev);
	p_file->private_data = dev;
	if ((p_file->f_flags & O_ACCMODE) == O_WRONLY) {
		// INIT OPERATIONS (IF NEEDED)
	}
	return 0;
}

/*****************************************************/
// MMAP
int pim_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int status;
	ulong_t off = vma->vm_pgoff << PAGE_SHIFT;
	ulong_t vsize = vma->vm_end - vma->vm_start;
	ulong_t psize = PHY_SIZE - off;
	PIM_INFO("pim_mmap");
	
	if (vsize > psize)
		return -EINVAL; /*  spans too high */

	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	status = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vsize, vma->vm_page_prot);
	if (status)
		return -EAGAIN;

	return 0;
}

/*****************************************************/
// RELEASE
int pim_release(struct inode *p_inode, struct file * p_file)
{
	PIM_INFO("pim_release");
	//kfree(p_file->private_data);
	kfree(pages.physical_addr);
	kfree(pages.list);
	return 0;
}

/*****************************************************/
module_init( pim_init);
module_exit( pim_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erfan Azarkhish");
MODULE_DESCRIPTION("PIM Device Driver");

/*****************************************************/
/********************UTILITIES************************/
/*****************************************************/

/*
  Pin user pages into memory
  @addr_start: address of the first page (must be page aligned)
  @n_pages: number of pages to pin

  Resulting pages are stored in pages global variable
  On success returns 0
*/
int pim_get_user_pages(ulong_t addr_start, ulong_t n_pages)
{
	ulong_t result;
	unsigned write = 1;	// TODO: Later TLB protection
	#ifdef DEBUG_DRIVER
	max_user_va = addr_start + n_pages*PAGE_SIZE -1;
	PIM_INFO("Maximum VA used by the user: 0x%lx", max_user_va);
	#endif

	// what get_user_pages returns
	pages.list = (struct page **)kmalloc((size_t)(n_pages*sizeof(struct page *)),GFP_KERNEL);
	if (pages.list == NULL)
	{
		PIM_WARN("Memory allocation failed!");
		return -ENOMEM;
	}

	// get pointers to user-space buffers and lock them into memory
	PIM_INFO("Trying to get mmap_sem ...");
	down_read(&current->mm->mmap_sem);
	result = get_user_pages(current, current->mm, addr_start, n_pages, write, 0, pages.list, NULL);
	up_read(&current->mm->mmap_sem);
	if (result != n_pages)
	{
		PIM_WARN("Could not get requested user-space virtual addresses");
		PIM_WARN("   Requested: %ld, Obtained: %ld", n_pages, result); 
		return -ENOMEM;
	}
	PIM_INFO("Successfully pinned %ld user pages.", n_pages);
	pages.count = n_pages;
	return 0;
}

/*
  Create a Page Table to be used by the PIM device. This structure is 
  placed in the main memory and a pointer to it is passed to PIM. Whenever
  a miss occurs in the TLB of PIM, It should access this structure and 
  update its TLB rules based on this structure
  @vaddr_start: start address of this slice table in the user virtual address
  Notice: This slice table is associated with a contiguous virtual region
*/

int pim_create_slice_table(ulong_t vaddr_start)
{
	unsigned i, j, c, order;
	//ulong_t* p;
	// void * kaddr;
	ASSERT(pages.count);

	// Allocate some space for holding the physical addresses of the pages
	pages.physical_addr = (ulong_t*)kmalloc(pages.count*sizeof(ulong_t), GFP_KERNEL);
	ASSERT_RET(pages.physical_addr, "kmalloc failed (pages.physical_addr)", -ENOMEM);

	/* Get the physical address for all pages */
	for (i=0; i<pages.count; i++) 
	{
		pages.physical_addr[i] = (ulong_t)page_to_phys(pages.list[i]);
		//PIM_INFO("PAGE[%d]: PHYSICAL ADDR: 0x%lx  PAGE STRUCT: 0x%lx", i, pages.physical_addr[i], (ulong_t)pages.list[i] );
	}

	/* 
	  Merge the contiguous pages to create slices
	  Note: even though, the number of slices can be less than the number of pages 
	  because of merging, to speed-up the procedure, we allocate a slice table equal
	  to the number of pages.

	  Notice: SliceTable also must be aligned to cache boundaries, otherwise it won't
	  be flushed properly
	 */
	order = PAGE_ORDER(pages.count*sizeof(Slice));
	ASSERT(order <= 10); // Limit of the OS
    slicetable = (Slice*)__get_free_pages(GFP_KERNEL, order);

    ASSERT_RET(slicetable, "__get_free_pages failed (slicetable)", -ENOMEM);
	slicetable_paddr = (ulong_t)virt_to_phys(slicetable);

    i=0; // I
    j=0; // J
    stat_numslices = 0;
	while (i<pages.count)
	{
        c=1;
        #ifdef PIM_MERGE_SLICES
        if( i<pages.count-1 )
            while (pages.physical_addr[i+c-1] + PAGE_SIZE == pages.physical_addr[i+c]) c++; // Number of contiguous slices up to now
        #endif

        // Merge the contiguous slices
        for (j=0; j<c; j++)
        {
            slicetable[i+j].vaddr = (ulong_t)(vaddr_start + PAGE_SIZE*(i)); // + PIM_HOST_OFFSET; // Translated to PIM VA;		(REMOVED BY ERFAN)
            slicetable[i+j].paddr = (ulong_t)(pages.physical_addr[i]);
            slicetable[i+j].size = PAGE_SIZE*(c);
        }
        stat_numslices++;
        i += c;
	}

	// Print the Slice Table
	PIM_INFO("-------------------------PIM SLICE TABLE-----------------------------------");
	PIM_INFO("  Page Order: %d", order);
	for (i=0; i<pages.count; i++) 
		PIM_INFO("0x%lx: SLICE[%d]: VA [0x%lx, 0x%lx] ---> PA [0x%lx, 0x%lx]  %ld(B)", (ulong_t)(&slicetable[i]),
        i, (ulong_t)slicetable[i].vaddr, (ulong_t)(slicetable[i].vaddr+slicetable[i].size-1), (ulong_t)slicetable[i].paddr,
        ((ulong_t)slicetable[i].paddr+slicetable[i].size-1), slicetable[i].size);
	PIM_INFO("---------------------------------------------------------------------------");

    // Check if the contiguous regions have been merged
    #ifdef DEBUG_DRIVER
    c=0;
    for (i=0; i<pages.count-1; i++) 
        if (pages.physical_addr[i] + PAGE_SIZE == pages.physical_addr[i+1]) c++; // Number of contiguous slices up to now
    PIM_INFO("#pages:%d   #slices:%d   #merges:%d", pages.count, stat_numslices, c);

    // p = (ulong_t*)slicetable;
    // for (i=0; i<pages.count*3; i++)
	   //  PIM_INFO(" @ 0x%lx : 0x%lx", (ulong_t)&p[i], p[i]);
	PIM_INFO("---------------------------------------------------------------------------");
    #endif

	/* 
	  Send the physical pointer to the SliceTable to PIM, so PIM can use it
	   whenever a miss occurs in its TLBs.
	*/
    pim_write_ulong_t(PIM_SLICETABLE,  slicetable_paddr);
    pim_write_ulong_t(PIM_SLICECOUNT,  pages.count);
    pim_write_ulong_t(PIM_SLICEVSTART, slicetable[0].vaddr);
	// Debug:
	PIM_INFO("PIM_SLICETABLE:  0x%lx", pim_read_ulong_t(PIM_SLICETABLE));
	PIM_INFO("PIM_SLICECOUNT:    %ld", pim_read_ulong_t(PIM_SLICECOUNT));
	PIM_INFO("PIM_SLICEVSTART: 0x%lx", pim_read_ulong_t(PIM_SLICEVSTART));

	/*
	  Flush the caches to make sure the most recent data is accessible by 
	  the PIM device.
	  Notice: cache flushing must be the last step, otherwise some of the
	  changes won't be reflected
	*/
	//pim_write_byte(PIM_M5_REG, PIM_TIME_STAMP + 3);
	pim_cache_flush();
	//pim_write_byte(PIM_M5_REG, PIM_TIME_STAMP + 4);
	
	/**** Do not put anything here ****/
	return 0;
}

/*
  Direct read and write to the memory mapped region of PIM
  These two functions directly communicate with PIM's SPM
  @offset: byte offset from the start of the mmaped region
  @data: can be byte or word
*/
void pim_write_byte(ulong_t offset, char data)
{
	ASSERT( offset < PHY_SIZE );
	((char*)pimdevice.mem)[offset] = data;
}
char pim_read_byte(ulong_t offset)
{
	ASSERT( offset < PHY_SIZE );
	return ((char*)pimdevice.mem)[offset];
}
void pim_write_ulong_t(ulong_t offset, ulong_t data)
{
	ASSERT( offset < PHY_SIZE );
	*((ulong_t*)((char*)pimdevice.mem+offset)) = data;
}
ulong_t pim_read_ulong_t(ulong_t offset)
{
	ASSERT( offset < PHY_SIZE );
	return *((ulong_t*)((char*)pimdevice.mem+offset));
}

/*
  Flush the caches to make sure the most recent data in the SliceTable
  is accessible by the PIM device.
  For every page in slicetable, it will flush the whole range
*/
int pim_cache_flush()
{
	/*
	gem5 does not implement the cache flush instructions of ARM.
	So we use a simulation hack to do it.
	*/
	ulong_t i;
	PIM_INFO("Flushing the caches using a simulation hack");
	for (i=0; i<pages.count; i++) 
	{
		//PIM_INFO("Flushed L2: pages.physical_addr[%ld]=0x%lx", i, pages.physical_addr[i]);
		pim_write_ulong_t(PIM_M5_D1_REG, (ulong_t)(pages.physical_addr[i]));
        pim_write_ulong_t(PIM_M5_D2_REG, (ulong_t)(pages.physical_addr[i]+PAGE_SIZE-1));
        // This is necessary to maintain the order of the transactions
        pim_read_ulong_t(PIM_M5_D1_REG);
        pim_read_ulong_t(PIM_M5_D2_REG);
		pim_write_byte(PIM_M5_REG, PIM_HOST_CACHE_FLUSH_HACK);
	}
	PIM_INFO("Flushing the slice-table itself");
	pim_write_ulong_t(PIM_M5_D1_REG, (ulong_t)(slicetable_paddr));
	pim_write_ulong_t(PIM_M5_D2_REG, (ulong_t)(slicetable_paddr + pages.count*sizeof(Slice)-1));
    // This is necessary to maintain the order of the transactions
    pim_read_ulong_t(PIM_M5_D1_REG);
    pim_read_ulong_t(PIM_M5_D2_REG);
	pim_write_byte(PIM_M5_REG, PIM_HOST_CACHE_FLUSH_HACK);

//  PIM_INFO("Flushing the slice-table itself");
//	void * kaddr;
//	PIM_INFO("Flushing the caches for user pages ...");
//	for (i=0; i<pages.count; i++) 
//	{
//		// Make sure that the lower bits are zero
//		ASSERT_DBG( GET_BIT_RANGE(pages.physical_addr[i], PAGE_SHIFT-1, 0) == 0 );
//
//		/* 
//		  create a kernel-space mapping, the L1 cache maintenance functions
//		  work on kernel virtual addresses. They only exist for low memory
//		 */
//
//		kaddr = kmap(pages.list[i]);
//		ASSERT_RET(kaddr!=NULL, "kmap failed", -ENOMEM);
//		__cpuc_flush_dcache_area(kaddr,PAGE_SIZE);	// Flush the L1 Data Cache
//		kunmap(pages.list[i]); // destroy kernel-space mapping
//		PIM_INFO("Flushed L1: pages.physical_addr[%ld]=0x%lx kaddr=0x%lx", i, pages.physical_addr[i], (ulong_t)kaddr );
//
//		/*
//		  Now, the L2 cache maintenance functions work on physical addresses
//		  So for them, we use the already extracted physical address of pages
//		*/
//		outer_clean_range((ulong_t)pages.physical_addr[i],(ulong_t)pages.physical_addr[i]+PAGE_SIZE);
//		//PIM_INFO("Flushed L2: pages.physical_addr[%ld]=0x%lx", i, pages.physical_addr[i]);
//	}
//
//	/*
//	  We must flush the slice-table, as well
//	*/
//	ASSERT_DBG(slicetable);
//	PIM_INFO("Flushing the caches for the slice-table itself ... PA:[0x%lx, 0x%lx]", slicetable_paddr, slicetable_paddr + pages.count*sizeof(Slice));
//
//	__cpuc_flush_dcache_area(slicetable, pages.count*sizeof(Slice));	// Flush the L1 Data Cache
//	outer_clean_range(slicetable_paddr, slicetable_paddr + pages.count*sizeof(Slice));

	PIM_INFO("Cache Flush Done!");
	return 0;
}
