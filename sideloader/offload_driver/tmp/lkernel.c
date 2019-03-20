#include <asm/apic.h>
#include <asm/tlbflush.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mc146818rtc.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/highmem.h>

//#include "../../include/arch.h"
#include "arch.h"
#include "page_32.h"

#include "../include/__boot_param.h"
#include "../include/localapic.h"
#include "../ripc/ripc.h"

#define LK_DEVICE_NAME "lk"

#define PAGE_4K		(0x1000)

#define LKLOADER	0
#define LK_IMG_SIZE	1
#define LKLOADING	3

#define LK_TEST		10

#define LK_PRINTS	20
#define CORE_STATUS	25
#define IPC_STATUS	26

#define CPU_ON		100
#define CPU_OFF		110

#define LKLOADER_TEST	201
#define SECTOR		512
//#define MAX_CORE	220
#define MAX_CORE	226

#define IO_REMAP	300

extern int lkernel_sysfs_init(void);
extern void lkernel_sysfs_exit(void);

static unsigned short g_onoff_bitmap[MAX_CORE];

static int lk_major = 0;
static struct class *lk_class;

/* unit = 512 byte */
static unsigned short g_kernel32 = 0;
static unsigned short g_kernel64 = 0;
static unsigned short g_total = 0;

unsigned long g_pml_addr = 0;
static struct page *g_ipcs_page = 0;
unsigned long g_vcon_addr = 0;
unsigned long g_boot_addr = 0;
unsigned long g_va_boot_addr = 0;

unsigned long g_shared_memory = 0 ;

static int lk_open(struct inode *inode, struct file *filp);
static int lk_release(struct inode *inode, struct file *filep);
static long lk_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static unsigned long lk_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations lk_fops = {
  .open = lk_open,
  .release = lk_release,
  .unlocked_ioctl = lk_ioctl,
  .mmap = lk_mmap,
};

/*
 * open lkernel
 */
static int lk_open(struct inode *inode, struct file *filp)
{
  return 0;
}

/*
 * send APIC IPI
 */
static int apic_send_ipi(unsigned int dest_shorthand, unsigned int dest,
                         int vector)
{
  unsigned send_status = 0, accept_status = 0;

  printk(KERN_DEBUG "send_ipi (%d)\n", dest);

  apic_icr_write(APIC_TRIGGER_MODE_EDGE | APIC_LEVEL_ASSERT |
                 APIC_DESTINATION_MODE_PHYSICAL | APIC_DELIVERY_MODE_FIXED |
                 49, dest);

  send_status = safe_apic_wait_icr_idle();
  accept_status = (apic_read(APIC_ESR) & 0xEF);

  printk("send %d, accept %d\n", send_status, accept_status);

  if (!send_status && !accept_status)
    pr_debug("ipi send failed (dest : %d), (vector : %d)\n", dest, vector);
  else
    pr_debug("ipi send success (dest : %d), (vector : %d)\n", dest,
             vector);

  return 0;
}

/*
 * wake up secondary cpu
 */
static int wakeup_secondary_cpu_via_init(int phys_apicid,
                                         unsigned long start_eip)
{

  int i = 0, num_starts = 0;
  unsigned long send_status = 0, accept_status = 0;
  unsigned long flags = 0;

  spin_lock_irqsave(&rtc_lock, flags);
  CMOS_WRITE(0xa, 0xf);
  spin_unlock_irqrestore(&rtc_lock, flags);
  local_flush_tlb();

  printk(KERN_INFO "Asserting INIT %lx for core 0x%x\n", start_eip,
         phys_apicid);
  apic_icr_write(APIC_INT_LEVELTRIG | APIC_INT_ASSERT | APIC_DM_INIT,
                 phys_apicid);

  /* Waiting for send to finish... */
  send_status = safe_apic_wait_icr_idle();
  mdelay(10);

  /* Deasserting INIT */
  apic_icr_write(APIC_INT_LEVELTRIG | APIC_DM_INIT, phys_apicid);

  /* Waiting for send to finish... */
  send_status = safe_apic_wait_icr_idle();

  //[20170407] mb();
  num_starts = 2;

  /* #startup loop */
  for (i = 1; i <= num_starts; i++) {

    apic_read(APIC_ESR);

    apic_icr_write(APIC_DM_STARTUP | (start_eip >> 12), phys_apicid);
    udelay(300);
    send_status = safe_apic_wait_icr_idle();
    udelay(300);
    accept_status = (apic_read(APIC_ESR) & 0xEF);
    if (send_status || accept_status)
      break;
  }

  /* After Startup */
  if (send_status)
    printk(KERN_INFO "APIC never delievered\n");
  if (accept_status)
    printk(KERN_INFO "APIC delivery error (%lx)\n", accept_status);

  return (send_status || accept_status);
}

/*
 * ioctl lkernel
 */
static long lk_ioctl(struct file *filp, unsigned int cmd,
                     unsigned long arg)
{
  int retu = 0;
  char *addr = NULL, *bladdr = NULL, lkinfo[6], *lkbin = NULL;

  memset(lkinfo, 0, sizeof(char) * 6);

  switch (cmd) {
  case LK_IMG_SIZE:
    retu = copy_from_user(lkinfo, (const void __user *) arg, 6);
    if (retu)
      return -1;

    g_total = *((unsigned short *) (lkinfo + 0));
    g_kernel32 = *((unsigned short *) (lkinfo + 2));
    g_kernel64 = *((unsigned short *) (lkinfo + 4));

    printk(KERN_INFO
           "sideloader: g_total %d g_kernel32: %d g_kernel64: %d uthread %d\n",
           g_total, g_kernel32, g_kernel64, (g_total - g_kernel32 - g_kernel64));
    break;

  case LKLOADING:
    lkbin = vmalloc((g_total) * SECTOR);
    if (lkbin == NULL) {
      printk(KERN_INFO "sideloader: allocation failed %d \n",
             (g_total) * SECTOR);
      return -1;
    };

    retu = copy_from_user(lkbin, (const void __user *) arg, (g_total) * SECTOR);

    if (retu) {
      printk(KERN_INFO "sideloader: copy_from_user failed\n");
      vfree(lkbin);
      return -1;
    };

    if (!g_boot_addr || !g_pml_addr) {
      vfree(lkbin);
      return -1;
    }

    //VCONSOLE
    if (!g_vcon_addr) {
      vfree(lkbin);
      return -1;
    }
    memset(__va(g_vcon_addr), 0x20, PAGE_4K);

    bladdr = __va(g_boot_addr);
    printk(KERN_INFO "lkloader copied to < 1M, [%p], size [%d]\n",
           __va(g_boot_addr), (g_kernel32) * SECTOR);

    memcpy(__va(g_boot_addr), lkbin, (g_kernel32) * SECTOR);
    //*((unsigned short *) (bladdr + BOOTSTRAPPROCESSOR_FLAG_OFFSET)) = 1;
    *((unsigned long *) (bladdr + PML4_ADDR_OFFSET)) = (g_pml_addr);
    *((unsigned long *) (bladdr + APIC_ADDR_OFFSET)) = APIC_DEFAULT_PHYS_BASE;
    *((unsigned long *) (bladdr + IPCS_ADDR_OFFSET)) =
        __pa(page_address(g_ipcs_page));

    // VCONSOLE
    *((unsigned long *) (bladdr + ADDR_VCON_OFFSET)) = (g_vcon_addr);
    printk(KERN_INFO "g_pml_addr %lx, apic_addr %lx\n",
           *((unsigned long *) (bladdr + PML4_ADDR_OFFSET))
           , *((unsigned long *) (bladdr + APIC_ADDR_OFFSET)));
    addr = ioremap(BASE_32 + 0x300000, (g_total - g_kernel32) * SECTOR);

    if (addr == NULL) {
      printk(KERN_INFO "ioremap error\n");
      vfree(lkbin);
      return -EINVAL;
    }
    memcpy(addr, lkbin + (g_kernel32) * SECTOR,
           (g_total - g_kernel32) * SECTOR);

    printk("from [%d] size [%d] bytes copied %p\n",
           (g_kernel32) * SECTOR, (g_total - g_kernel32) * SECTOR, addr);

    iounmap(addr);
    vfree(lkbin);
    break;

  case LK_PRINTS:
    if (!g_vcon_addr) return -1;
    retu = copy_to_user((void __user *) arg, __va(g_vcon_addr), PAGE_4K);
    break;

  case CORE_STATUS:
#ifdef DISPLAY_CORE_STATUS
    if (!g_boot_addr) return -1 ;
    retu = copy_to_user((void __user *) arg, __va(g_boot_addr + 3840), 220);
#endif
    break ;


  case IPC_STATUS:
#ifdef DISPLAY_IPC_STATUS
    if (!g_boot_addr) return -1 ;
    retu = copy_to_user((void __user *) arg, __va(g_boot_addr + 3840+220), 36);
#endif
    break ;

  case CPU_ON:
    {
#ifdef DISPLAY_CORE_STATUS
      // to display core status
      char *core;
#endif
      unsigned int phy;
      retu = copy_from_user(&phy, (const void __user *) arg,
                         sizeof(unsigned int));

      if (phy >= MAX_CORE || g_onoff_bitmap[phy] == 1)
        return -1;
      g_onoff_bitmap[phy] = 1;

#ifdef DISPLAY_CORE_STATUS
      // to display core status
      core  = __va(g_boot_addr + 3840 + phy);
      *core = '1' ;
#endif

      wakeup_secondary_cpu_via_init(phy, g_boot_addr);

      /*  check error code  */
    }
    break;

  case CPU_OFF:
    {
#ifdef DISPLAY_CORE_STATUS
      // to display core status
      char *core;
#endif
      unsigned int phy;
      retu = copy_from_user(&phy, (const void __user *) arg,
                            sizeof(unsigned int));

      if (phy >= MAX_CORE || g_onoff_bitmap[phy] == 0)
        return -1;
      g_onoff_bitmap[phy] = 0;

#ifdef DISPLAY_CORE_STATUS
      // to display core status
      core = __va(g_boot_addr + 3840 + phy);
      *core = '0' ;
#endif

      apic_send_ipi(0x00, phy, 49);

      /*  check error code */
    }
    break;

  case IO_REMAP:
  {
  struct addr_info {
	long long pa;
        int length;
  };
 // struct addr_info pa_addr;
  unsigned char *va_addr;

  
  unsigned long pa_addr;
  int pa_length = 1024;
    retu = copy_from_user(&pa_addr, (const void __user *) arg, sizeof(unsigned long));
    printk(KERN_INFO "ioremap pa: %ld, len:%d\n", pa_addr, pa_length);
    if (retu)
      return -1;
    va_addr = ioremap(BASE_32 + (300 << 20), pa_length);

    if (va_addr == NULL) {
      printk(KERN_INFO "ioremap error\n");
      vfree(lkbin);
      return -EINVAL;
    }
  }
  break;

  default:
    break;
  }
  return 0;
}

/*
 * relase lkernel
 */
static int lk_release(struct inode *inode, struct file *filep)
{
  return 0;
}


/*
 * return value:
 *  success : allocated physical memory address
 *  fail	: 0
*/

/*
static phys_addr_t reserve_memblock(phys_addr_t start, phys_addr_t end,
                                    phys_addr_t mem_size)
{
  phys_addr_t mem_loc;
  int ret;

  phys_addr_t(*f) (phys_addr_t, phys_addr_t, phys_addr_t, phys_addr_t);
  int (*g) (phys_addr_t, phys_addr_t);

  printk(KERN_DEBUG "memblock [ from (%llx), to (%llx), size(%llx) ]\n",
         (unsigned long long) start, (unsigned long long) end,
         (unsigned long long) mem_size);

  f = (phys_addr_t(*)(phys_addr_t, phys_addr_t, phys_addr_t, phys_addr_t))
      KERN_FUNC_MEMBLOCK_FIND_IN_RANGE;

  g = (int (*)(phys_addr_t, phys_addr_t))
      KERN_FUNC_MEMBLOCK_RESERVE;

  mem_loc = f(start, end, PAGE_ALIGN(mem_size), PAGE_4K);
  if (mem_loc == MEMBLOCK_ERROR) {
    printk(KERN_DEBUG "memblock [ fail to find ]\n");
    return 0;
  }

  ret = g(mem_loc, mem_size);

  printk(KERN_DEBUG "memblock [ alloc (%llx) ]\n",
         (unsigned long long) mem_loc);

  return mem_loc;
}

static void free_memblock(phys_addr_t addr, phys_addr_t size)
{
  int (*f) (phys_addr_t base, phys_addr_t size);

  f = (int (*)(phys_addr_t base, phys_addr_t size))
      KERN_FUNC_MEMBLOCK_FREE;

  f(addr, size);

  return;
}
*/

/*
 * set page entry
 */
void set_page_entry(PT_ENTRY * entry, DWORD upper_base_address,
                    DWORD lower_base_address, DWORD lower_flags,
                    DWORD upper_flags)
{
  entry->attribute_and_lower_base_address =
      lower_base_address | lower_flags;
  entry->upper_base_address_and_exb =
      (upper_base_address & 0xFF) | upper_flags;
}

/*
 * initialize page table
 */
static void init_page_table(QWORD base_va)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  QWORD mapping_address = 0, upper_address = 0;
  int i = 0;
  QWORD base_pa = BOOT_ADDR + PAGE_4K * 2;

  // initialize PML4T_ENTRY
  pml4t_entry = (PML4T_ENTRY *) base_va;

  for (i = 0; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pml4t_entry[i]), 0, 0, 0, 0);

  set_page_entry(&(pml4t_entry[0]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pml4t_entry[256]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  // initialize PDPT_ENTRY
  pdpt_entry = (PDPT_ENTRY *) (base_va + PAGE_4K);
  set_page_entry(&(pdpt_entry[0]), 0, (base_pa + 0x2000) + (0 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pdpt_entry[1]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[2]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[3]), 0, (base_pa + 0x2000) + (1 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  for (i = 4; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pdpt_entry[i]), 0, 0, 0, 0);

  // initialize PD_ENTRY
  pd_entry = (PD_ENTRY *) (base_va + 0x2000);

  mapping_address = 0;
  set_page_entry(&(pd_entry[0]), 0, mapping_address,
                 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

  mapping_address = ((QWORD) BASE_32 + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) BASE_32 + 0x200000) >> 32;
  for (i = 1; i < PAGE_MAX_ENTRY_COUNT; i++) {
    set_page_entry(&(pd_entry[i]), upper_address, mapping_address, 
		 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

    mapping_address += PAGE_DEFAULT_SIZE;
  }

/*
  virtual -> physical (256MB*3 = 768MB)
  0xC000_0000 -> 0x0000_0000
  ...
  0xEFFF_FFFF -> 0x2FFF_FFFF
*/

  mapping_address = 0;
  set_page_entry(&(pd_entry[(PAGE_MAX_ENTRY_COUNT)]),
                 (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12, mapping_address,
                 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

  mapping_address = ((QWORD) BASE_32 + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) BASE_32 + 0x200000) >> 32;

  for (i = PAGE_MAX_ENTRY_COUNT + 1; i < PAGE_MAX_ENTRY_COUNT + 128 * 3; i++) {
    set_page_entry(&(pd_entry[i]), upper_address,
                   mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS),
                   0);

    mapping_address += PAGE_DEFAULT_SIZE;
  }
/*
  It's for accessing APIC registers.

  virtual -> physical (256MB)
  0xF000_0000 -> 0xF000_0000
  ...
  0xFFFF_FFFF -> 0xFFFF_FFFF
*/
  mapping_address = 0xF0000000;
  for (; i < PAGE_MAX_ENTRY_COUNT * 2; i++) {
    DWORD dwPageEntryFlags;

    dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;

    set_page_entry(&(pd_entry[i]), (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12,
                   mapping_address, dwPageEntryFlags, 0);
    mapping_address += PAGE_DEFAULT_SIZE;
  }
}

/*
 * mmap lkernel
 */
static unsigned long lk_mmap(struct file *filp, struct vm_area_struct *vma)
{
  void *va = NULL;
  size_t vma_size = vma->vm_end - vma->vm_start;
  unsigned long long offset = vma->vm_pgoff << PAGE_SHIFT;
  vma->vm_flags |= VM_IO; // 
  vma->vm_flags |= VM_RESERVED;

  printk("1. remap start : %llx, off: %llx\n", (u64) vma->vm_start, (u64) vma->vm_pgoff);

  /*int order = get_order(vma_size);

  g_ipcs_page = alloc_pages(GFP_KERNEL, order);


    va = (void __force *) ioremap(vma->vm_pgoff << PAGE_SHIFT, vma_size);
    if (va == NULL) {
      printk(KERN_INFO "ioremap error\n");
      return -EINVAL;
    }
    unsigned long pfn = virt_to_bus(va) >> PAGE_SHIFT;

    remap_pfn_range(vma, vma->vm_start, pfn, vma_size, vma->vm_page_prot);

*/
    //remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma_size, vma->vm_page_prot);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 
	printk(KERN_INFO "off=%llx\n", vma->vm_pgoff); 
        remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, vma_size, vma->vm_page_prot); 


/*

  if (io_remap_pfn_range
      //(vma, vma->vm_start, page_to_pfn(g_ipcs_page), vma_size,
      (vma, vma->vm_start, (u64) vma->vm_pgoff << PAGE_SHIFT, vma_size,
       vma->vm_page_prot) < 0) {
    printk("remap failed\n");
    printk(KERN_DEBUG "g_ipcs_page: %llx\n", (unsigned long long)g_ipcs_page);
    printk(KERN_DEBUG, "remap ok pfn : %llx\n", (u64) page_address(g_ipcs_page));
    return -EAGAIN;
  }
*/
  //printk("remap ok pfn : %llx, %llx\n", (u64) page_address(g_ipcs_page), (u64) g_ipcs_page);
  printk("2. remap start : %llx, off: %llx\n", (u64) vma->vm_start, (u64) vma->vm_pgoff);
  //printk(KERN_DEBUG, "remap ok pfn : %llx\n", (u64) page_address(g_ipcs_page));

  return 0;
}

/*
 * initialize lkernel
 */
static int __init lk_init(void)
{
  struct device *err_dev = NULL;
  //int order = XXX(vma->vm_end - vma->vm_start);
  int order = get_order(REMOTE_PAGE_MEMORY_SIZE * PAGE_SIZE);

  g_ipcs_page = alloc_pages(GFP_KERNEL, order);
  if (!g_ipcs_page) {
    printk(KERN_ERR "failed to allocate memory\n");
    return -1;
  }

  //g_boot_addr = reserve_memblock(0, 0x100000, PAGE_4K * 6);
  g_boot_addr = BOOT_ADDR;
  g_va_boot_addr = (unsigned long) ioremap(g_boot_addr, PAGE_4K * 6);
  if(g_va_boot_addr == (unsigned long) NULL) {
    printk(KERN_ERR "unable to ioremap for %llx\n", (unsigned long long) g_boot_addr);
    return -1;
  }

  // VCONSOLE
  g_vcon_addr = g_boot_addr + PAGE_4K;

  g_pml_addr = g_boot_addr + PAGE_4K * 2;
  //init_page_table((unsigned long) __va(g_pml_addr));
  init_page_table((unsigned long) (g_va_boot_addr + PAGE_4K * 2));

#ifdef DISPLAY_CORE_STATUS
  // to display core status
  memset(__va(g_boot_addr+3840), '0', MAX_CORE) ;
  memset(__va(g_boot_addr+3840+MAX_CORE),0,256-MAX_CORE) ;
#endif

  printk(KERN_DEBUG "g_boot_addr: %llx, g_vcon_addr: %llx, g_pml_addr : %llx\n",
         (unsigned long long) g_boot_addr, (unsigned long long) g_vcon_addr,
         (unsigned long long) g_pml_addr);

  lk_major = register_chrdev(0, LK_DEVICE_NAME, &lk_fops);
  if (lk_major < 0) {
    printk(KERN_ERR "unable to register LK : %d\n", lk_major);
    return -1;
  }

  lk_class = class_create(THIS_MODULE, LK_DEVICE_NAME);
  err_dev = device_create(lk_class, NULL, MKDEV(lk_major, 0), NULL,
                          LK_DEVICE_NAME);

  printk(KERN_INFO "lk: register device at major %d\n", lk_major);


  if ( lkernel_sysfs_init()) {
    printk(KERN_ERR "unable to create sysfs\n");
  }

  return 0;
}

/*
 * exit lkernel
 */
void lk_exit(void)
{
  __free_pages(g_ipcs_page, get_order(PAGE_SIZE * REMOTE_PAGE_MEMORY_SIZE));
  iounmap((void *) g_boot_addr);

  device_destroy(lk_class, MKDEV(lk_major, 0));
  class_unregister(lk_class);
  class_destroy(lk_class);
  unregister_chrdev(lk_major, LK_DEVICE_NAME);

  printk(KERN_INFO "lk: removing device at major %d\n", lk_major);

  lkernel_sysfs_exit();
}

module_init(lk_init)
module_exit(lk_exit)

MODULE_LICENSE("GPL");
