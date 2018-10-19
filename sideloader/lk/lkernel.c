#include "lkernel.h"
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
#include <linux/vmalloc.h>

#include "arch.h"
#include "cmds.h"

#include "../include/localapic.h"
#include "../include/page.h"

#define LK_DEVICE_NAME "lk"

static unsigned short g_onoff_bitmap[MAX_PROCESSOR_COUNT];

static int lk_major = 0;
static struct class *lk_class;

/* unit = 512 byte */
static unsigned short g_kernel32 = 0;
static unsigned short g_kernel64 = 0;
static unsigned short g_uapp = 0;
static unsigned short g_total = 0;

static unsigned short g_vcon[VCON_SIZE] = {0, };

static unsigned int start_index, core_start, core_end;
static unsigned long memory_start, memory_end, memory_start_addr;

unsigned long g_pml_addr = 0;
static struct page *g_ipcs_page = 0;
unsigned long g_boot_addr = 0;
unsigned long g_va_boot_addr = 0;

static SHELL_STORAGE_AREA *g_shell_addr;
static char *g_addr;
static TCB *g_tcb_addr[MAX_PROCESSOR_COUNT+CONFIG_NUM_THREAD];
static char *g_log;
static unsigned long g_check_memory;

static int lk_open(struct inode *inode, struct file *filp);
static int lk_release(struct inode *inode, struct file *filep);
static long lk_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int lk_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations lk_fops = {
  .open = lk_open,
  .release = lk_release,
  .unlocked_ioctl = lk_ioctl,
  .mmap = lk_mmap,
};

/**
 * Allocate vcon
 * 0: usable, 1: used
 */
int alloc_vcon(void)
{
  int vcon_num = -1;
  int i;

  for (i=0; i<VCON_SIZE; i++) {
    if (g_vcon[i] == 0) {
      vcon_num = i;
      g_vcon[i] = 1;
      break;
    }
  }

  return vcon_num;
}

/**
 * Free vcon : Set the input number of vcon to usable 
 */
int free_vcon(int loc)
{
  if (g_vcon[loc] == 1)
    g_vcon[loc] = 0;
  else
    return -1;

  return 0; 
}

/**
 * open lkernel
 */
static int lk_open(struct inode *inode, struct file *filp)
{
  return 0;
}

/**
 * send APIC IPI
 */
static int apic_send_ipi(unsigned int dest_shorthand, unsigned int dest, int vector)
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
    pr_debug("ipi send success (dest : %d), (vector : %d)\n", dest, vector);

  return 0;
}

/**
 * wake up secondary cpu
 */
static int wakeup_secondary_cpu_via_init(int phys_apicid, unsigned long start_eip)
{
  int i = 0, num_starts = 0;
  unsigned long send_status = 0, accept_status = 0;
  unsigned long flags = 0;

  spin_lock_irqsave(&rtc_lock, flags);
  CMOS_WRITE(0xF, 0xA);
//  CMOS_WRITE(0xa, 0xf);
  spin_unlock_irqrestore(&rtc_lock, flags);
  local_flush_tlb();

  printk(KERN_INFO "Asserting INIT %lx for core 0x%x\n", start_eip, phys_apicid);
  apic_icr_write(APIC_INT_LEVELTRIG | APIC_INT_ASSERT | APIC_DM_INIT, phys_apicid);

  // Waiting for send to finish...
  send_status = safe_apic_wait_icr_idle();
  mdelay(10);

  // Deasserting INIT
  apic_icr_write(APIC_INT_LEVELTRIG | APIC_DM_INIT, phys_apicid);

  // Waiting for send to finish...
  send_status = safe_apic_wait_icr_idle();

  // [20170407] mb();
  num_starts = 2;

  // #startup loop
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

  // After Startup
  if (send_status)
    printk(KERN_INFO "APIC never delievered\n");
  if (accept_status)
    printk(KERN_INFO "APIC delivery error (%lx)\n", accept_status);

  return (send_status || accept_status);
}

/**
 * ioctl lkernel
 * LK_PARAM: Save input parameters and initialize the pagetable
 * LK_IMAGE_SIZE: Get the size of the kernel images and save into the variables
 * LK_LOADING: Copy image file into the memory separated into bootloader, kernel, and application
 */
static long lk_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int retu = 0;

  switch (cmd) {
  case LK_PARAM:
  {
  // LK_PARAM: Save input parameters and initialize the pagetable
    unsigned short g_param[CONFIG_PARAM_NUM] = {0, };

    retu = copy_from_user(g_param, (const void __user *) arg, 10);
    if (retu)
      return -1;

    // Set cpu and memory information based on index
    start_index = g_param[0];
    if (start_index != 0) {
      core_start = (start_index*5+15);		// core_start
      core_end = (start_index*5+19);		// core_end
      memory_start = (start_index*10-5);	// memory_start
      memory_end = (start_index*10+5);		// memory_end
    } else {
      core_start = g_param[1];
      core_end = g_param[2];
      memory_start = g_param[3];
      memory_end = g_param[4];
    }
    memory_start_addr = memory_start * 1024 * 1024 * 1024;

    printk(KERN_INFO "LK_PARAM: index: %d, core_start: %d, core_end: %d, memory_start: %d, memory_end: %d\n",
           (int)start_index, (int)core_start, (int)core_end, (int)memory_start, (int)memory_end);
    printk(KERN_INFO "LK_PARAM: memory_start_addr: %lx\n", (unsigned long) memory_start_addr);

    // Pagetable Initialize
    g_boot_addr = BOOT_ADDR;
    g_va_boot_addr = (unsigned long) ioremap(g_boot_addr, PAGE_4K * 10);
    if(g_va_boot_addr == (unsigned long) NULL) {
      printk(KERN_ERR "LK_PARAM: unable to ioremap for %llx\n", (unsigned long long) g_boot_addr);
      return -1;
    }
 
    g_pml_addr = g_boot_addr + PAGE_4K;
    init_page_table((unsigned long) (g_va_boot_addr + PAGE_4K));

    printk(KERN_INFO "LK_PARAM: Pagetable initialization complete, g_boot_addr: %llx, g_pml_addr : %llx\n", (unsigned long long) g_boot_addr, (unsigned long long) g_pml_addr);

    // Ioremap of the shell storage
    g_addr = ioremap ((memory_start_addr + ((CONFIG_SHELL_STORAGE+5)*1024*1024)), sizeof (SHELL_STORAGE_AREA));
    if (g_addr == NULL) {
      printk (KERN_INFO "g_addr ioremap error\n");
      return -EINVAL;
    }
    printk (KERN_INFO "LK_PARAM: g_shell_addr ioremap success!!\n");
    g_shell_addr = (SHELL_STORAGE_AREA *) g_addr;

    g_log = ioremap ((memory_start_addr + (CONFIG_SHELL_STORAGE*1024*1024)+ LOG_LENGTH), (MAX_LOG_COUNT+1)*LOG_LENGTH);
    if (g_log == NULL) {
      printk (KERN_INFO "g_log ioremap error\n");
      return -EINVAL;
    }
    printk (KERN_INFO "LK_PARAM: g_log ioremap success!!\n");

//j    printk (KERN_INFO "*** %lx, %lx\n", g_shell_addr->thread_area[i], pa(g_shell_addr->thread_area[i]);
#if 0
    for (i = 0; i < MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD; i++) {
      g_tcb_addr[i] = ioremap(pa(g_shell_addr->thread_area[i]), sizeof(TCB));
      if (g_tcb_addr[i] == NULL) {
        printk (KERN_INFO "tcb ioremap error - %d\n", i);
        return -EINVAL;
      }
    }
    printk (KERN_INFO "LK_PARAM: g_tcb_addr ioremap success!!\n");
#endif
  }
    break;

  case LK_IMG_SIZE:
  {
  // LK_IMAGE_SIZE: Get the size of the kernel images and save into the variables
    char lkinfo[8] = {0, };

    retu = copy_from_user(lkinfo, (const void __user *) arg, 8);
    if (retu)
      return -1;

    g_total = *((unsigned short *) (lkinfo + 0));
    g_kernel32 = *((unsigned short *) (lkinfo + 2));
    g_kernel64 = *((unsigned short *) (lkinfo + 4));
    g_uapp = *((unsigned short *) (lkinfo + 6));

    printk(KERN_INFO
           "LK_SIZE: sideloader: g_total %d bootloader: %d g_kernel64: %d uthread %d\n",
           g_total, g_kernel32, g_kernel64, g_total - g_kernel32 - g_kernel64);
  }
    break;

  case LK_LOADING:
  {
  // LK_LOADING: Copy image file into the memory separated into bootloader, kernel, and application
    unsigned long vcon_addr;
    char *addr = NULL, *bladdr = NULL, *lkbin = NULL;

    // copy image file into the lkbin variable
    lkbin = vmalloc((g_total) * SECTOR);
    if (lkbin == NULL) {
      printk(KERN_INFO "LK_LOADING: allocation failed %d \n", (g_total) * SECTOR);
      return -1;
    };

    retu = copy_from_user(lkbin, (const void __user *) arg, (g_total) * SECTOR);

    if (retu) {
      printk(KERN_INFO "LK_LOADING: copy_from_user failed\n");
      vfree(lkbin);
      return -1;
    };

    if (!g_boot_addr || !g_pml_addr) {
      vfree(lkbin);
      return -1;
    }

    // Initialize vcon address
    vcon_addr = alloc_vcon();
    if (vcon_addr == -1) {
      vfree(lkbin);
      printk(KERN_INFO "LK_LOADING: vcon address is not allocated\n");

      return -1;
    }
    vcon_addr = g_boot_addr + PAGE_4K * (vcon_addr+VCON_INDEX);
    memset(__va(vcon_addr), 0x20, PAGE_4K);

    // Bootloader
    bladdr = __va(g_boot_addr);
    printk(KERN_INFO "LK_LOADING: bootloader copied to < 1M, [%p], size [%d]\n", __va(g_boot_addr), (g_kernel32) * SECTOR);

    memcpy(__va(g_boot_addr), lkbin, (g_kernel32) * SECTOR);

    // Metadata
    *((unsigned long *) (bladdr + META_OFFSET)) = (vcon_addr);
    *((unsigned long *) (bladdr + META_OFFSET + PML4_OFFSET)) = g_pml_addr;
    *((unsigned long *) (bladdr + META_OFFSET + APIC_OFFSET)) = APIC_DEFAULT_PHYS_BASE;
    *((unsigned long *) (bladdr + META_OFFSET + CPU_START_OFFSET)) = core_start;
    *((unsigned long *) (bladdr + META_OFFSET + CPU_END_OFFSET)) = core_end;
    *((unsigned long *) (bladdr + META_OFFSET + MEMORY_START_OFFSET)) = memory_start;
    *((unsigned long *) (bladdr + META_OFFSET + MEMORY_END_OFFSET)) = memory_end;
    *((unsigned long *) (bladdr + META_OFFSET + QEMU_OFFSET)) = 0;

    printk(KERN_INFO "LK_LOADING: VCON_ADDR: %lx\n", vcon_addr);
    printk(KERN_INFO "LK_LOADING: CPU_START: %d, CPU_END: %d\n", core_start, core_end);
    printk(KERN_INFO "LK_LOADING: MEMORY_START: %d GB, MEMORY_END: %d GB\n", (int) memory_start, (int) memory_end);
    printk(KERN_INFO "LK_LOADING: g_pml_addr %lx, apic_addr %lx\n", (unsigned long) g_pml_addr, (unsigned long) APIC_DEFAULT_PHYS_BASE);

    // Kernel
    addr = ioremap_nocache(memory_start_addr + KERNEL_ADDR, (g_total - g_kernel32) * SECTOR);

    if (addr == NULL) {
      printk(KERN_INFO "LK_LOADING: Kernel ioremap error\n");
      vfree(lkbin);
      return -EINVAL;
    } else {
      printk(KERN_INFO "LK_LOADING: Kernel ioremap success: %lx, %d\n", (unsigned long) addr, (g_total-g_kernel32) * SECTOR);
    }

    memcpy(addr, lkbin + (g_kernel32 * SECTOR), g_kernel64 * SECTOR);

    printk("LK_LOADING: Kernel copied from [%d] size [%d] bytes copied %p\n",
           (g_kernel32) * SECTOR, (g_total - g_kernel32) * SECTOR, addr);

    // User application
    addr = ioremap_nocache(memory_start_addr + APP_ADDR, (g_total - g_kernel32 - g_kernel64) * SECTOR);

    if (addr == NULL) {
      printk(KERN_INFO "LK_LOADING: Application ioremap error\n");
      vfree(lkbin);
      return -EINVAL;
    } else {
      printk(KERN_INFO "LK_LOADING: Appliation ioremap success: %lx, %d\n", (unsigned long) addr, (g_total-g_kernel32) * SECTOR);
    }

    memcpy(addr, lkbin + (g_kernel32 * SECTOR) + (g_kernel64 * SECTOR), (g_total - g_kernel32 - g_kernel64) * SECTOR);

    printk("LK_LOADING: Application copied from [%d] size [%d] bytes copied %p\n",
           (g_kernel32+g_kernel64) * SECTOR, (g_total - g_kernel32 - g_kernel64) * SECTOR, addr);

    iounmap(addr);
    vfree(lkbin);
  }
    break;

  case CPU_ON:
  {
    unsigned int phy;
    retu = copy_from_user(&phy, (const void __user *) arg, sizeof(unsigned int));

    if (phy >= MAX_PROCESSOR_COUNT || g_onoff_bitmap[phy] == 1)
      return -1;
    g_onoff_bitmap[phy] = 1;

    wakeup_secondary_cpu_via_init(phy, g_boot_addr);

    // TODO: check error code
  }
    break;

  case CPU_OFF:
  {
    unsigned int phy;
    retu = copy_from_user(&phy, (const void __user *) arg, sizeof(unsigned int));

    if (phy >= MAX_PROCESSOR_COUNT || g_onoff_bitmap[phy] == 0)
      return -1;
    g_onoff_bitmap[phy] = 0;

    apic_send_ipi(0x00, phy, 49);

    // TODO: check error code
  }
    break;

  case IO_REMAP:
  {
    struct addr_info {
      long long pa;
      int length;
    };
    unsigned char *va_addr;
    unsigned long pa_addr;
    int pa_length = 1024;

    retu = copy_from_user(&pa_addr, (const void __user *) arg, sizeof(unsigned long));
    printk(KERN_INFO "IO_REMAP: ioremap pa: %ld, len:%d\n", pa_addr, pa_length);
    if (retu)
      return -1;
    va_addr = ioremap(memory_start_addr + (300 << 20), pa_length);

    if (va_addr == NULL) {
      printk(KERN_INFO "IO_REMAP: ioremap error\n");
//      vfree(lkbin);
      return -EINVAL;
    }
  }
    break;

  case LK_CMD_PAGEFAULT:
    if ((retu = copy_to_user((char *)arg, (char *)&g_shell_addr->pf_area, sizeof(g_shell_addr->pf_area))) < 0) {
      printk (KERN_INFO "LK_CMD_PAGEFAULT: copy_to_user error\n");
      return -EINVAL;
    }

    printk (KERN_INFO "PAFA copied\n");
    break;

  case LK_CMD_STAT:
    if ((retu = copy_to_user ((char *) arg, (char *) &g_shell_addr->init_stat, sizeof (g_shell_addr->init_stat))) < 0) {
      printk (KERN_INFO "LK_CMD_STAT: copy_to_user error\n");
      return -EINVAL;
    }

    printk (KERN_INFO "STAT copied\n");
    break;

  case LK_CMD_THREAD:
  {
    TCB *tcb_ptr = (TCB *) arg;
    int i;

#if 1
    if(g_check_memory != memory_start_addr) {
      for (i = 0; i < MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD; i++) {
        g_tcb_addr[i] = ioremap(pa(g_shell_addr->thread_area[i]), sizeof(TCB));
        if (g_tcb_addr[i] == NULL) {
          printk (KERN_INFO "tcb ioremap error - %d\n", i);
          return -EINVAL;
        }
      }
      printk (KERN_INFO "LK_PARAM: g_tcb_addr ioremap success!!\n");
      g_check_memory = memory_start_addr;
    } 
#endif

    for (i=0; i<MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD; i++) {
      if ((retu = copy_to_user ((TCB *)&tcb_ptr[i], (TCB *)g_tcb_addr[i], sizeof(TCB))) < 0) {
        printk (KERN_INFO "LK_CMD_THREAD: copy_to_user error : %d\n", i);
        return -EINVAL;
      }
    }

    printk (KERN_INFO "THREAD copied\n");
  }
    break;

  case LK_CMD_TIMER:
    if ((retu = copy_to_user ((char *) arg, (char *) g_shell_addr->timer, sizeof (g_shell_addr->timer))) < 0) {
      printk (KERN_INFO "LK_CMD_TIMER: copy_to_user error\n");
      return -EINVAL;
    }

    printk (KERN_INFO "TIMER copied\n");
    break;

  case LK_CMD_CONSOLE:
  {
    unsigned long vcon_start_addr = BOOT_ADDR + PAGE_4K*VCON_INDEX; 
    retu = copy_to_user((void __user *) arg, __va(vcon_start_addr), PAGE_4K*VCON_SIZE);
  }
    break;

  case LK_CMD_LOG:
    if ((retu = copy_to_user ((char *) arg, (char *) g_log, (MAX_LOG_COUNT+1)*LOG_LENGTH)) < 0) {
      printk (KERN_INFO "LK_CMD_LOG: copy_to_user error\n");
      return -EINVAL;
    }

    printk (KERN_INFO "LOG copied\n");
    break; 

  default:
    break;
  }
  return 0;
}

/**
 * relase lkernel
 */
static int lk_release(struct inode *inode, struct file *filep)
{
  return 0;
}

/**
 * Set page entry for page table
 */
void set_page_entry(PT_ENTRY * entry, DWORD upper_base_address,
                    DWORD lower_base_address, DWORD lower_flags,
                    DWORD upper_flags)
{
  entry->attribute_and_lower_base_address = lower_base_address | lower_flags;
  entry->upper_base_address_and_exb = (upper_base_address & 0xFF) | upper_flags;
}

/**
 * Initialize page table
 * Use 4 pages (PML4T, PDPT, PD, PD) from base address (BOOT_ADDR + 4K)
 *   - input: page table address (virtual)
 */
void init_page_table(QWORD base_va)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  QWORD mapping_address = 0, upper_address = 0;
  int i = 0;
  QWORD base_pa = BOOT_ADDR + PAGE_4K;

  // Initialize PML4T_ENTRY
  pml4t_entry = (PML4T_ENTRY *) base_va;

  for (i = 0; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pml4t_entry[i]), 0, 0, 0, 0);

  set_page_entry(&(pml4t_entry[0]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pml4t_entry[256]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  // Initialize PDPT_ENTRY
  pdpt_entry = (PDPT_ENTRY *) (base_va + PAGE_4K);
  set_page_entry(&(pdpt_entry[0]), 0, (base_pa + 0x2000) + (0 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pdpt_entry[1]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[2]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[3]), 0, (base_pa + 0x2000) + (1 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  for (i = 4; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pdpt_entry[i]), 0, 0, 0, 0);

  // Initialize PD_ENTRY
  pd_entry = (PD_ENTRY *) (base_va + 0x2000);

  mapping_address = 0;
  set_page_entry(&(pd_entry[0]), 0, mapping_address,
                 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

  mapping_address = ((QWORD) memory_start_addr + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) memory_start_addr + 0x200000) >> 32;
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

  mapping_address = ((QWORD) memory_start_addr + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) memory_start_addr + 0x200000) >> 32;

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
static int lk_mmap(struct file *filp, struct vm_area_struct *vma)
{
  size_t vma_size = vma->vm_end - vma->vm_start;
  unsigned long long offset = vma->vm_pgoff << PAGE_SHIFT;

  vma->vm_flags |= VM_IO; // 
  vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);

  printk("1. remap start : %llx, off: %llx\n", (u64) vma->vm_start, (u64) vma->vm_pgoff);

  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 
  printk(KERN_INFO "off=%llx\n", (unsigned long long) vma->vm_pgoff); 
  remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, vma_size, vma->vm_page_prot); 

  printk("2. remap start : %llx, off: %llx\n", (u64) vma->vm_start, (u64) vma->vm_pgoff);

  return 0;
}

/*
 * Initialize lkernel module
 */
static int __init lk_init(void)
{
  struct device *err_dev = NULL;
  //int order = XXX(vma->vm_end - vma->vm_start);
  int order = get_order(REMOTE_PAGE_MEMORY_SIZE * PAGE_SIZE);

  g_ipcs_page = alloc_pages(GFP_KERNEL, order);
  if (!g_ipcs_page) {
    printk(KERN_ERR "INIT: failed to allocate memory\n");
    return -1;
  }

  lk_major = register_chrdev(0, LK_DEVICE_NAME, &lk_fops);
  if (lk_major < 0) {
    printk(KERN_ERR "INIT: unable to register LK : %d\n", lk_major);
    return -1;
  }

  lk_class = class_create(THIS_MODULE, LK_DEVICE_NAME);
  err_dev = device_create(lk_class, NULL, MKDEV(lk_major, 0), NULL, LK_DEVICE_NAME);

  printk(KERN_INFO "INIT: register device at major %d\n", lk_major);

  return 0;
}

/*
 * exit lkernel module
 */
void lk_exit(void)
{
  // TODO
  __free_pages(g_ipcs_page, get_order(PAGE_SIZE * REMOTE_PAGE_MEMORY_SIZE));
  iounmap((void *) g_boot_addr);
  iounmap((void *) g_addr);

  device_destroy(lk_class, MKDEV(lk_major, 0));
  class_unregister(lk_class);
  class_destroy(lk_class);
  unregister_chrdev(lk_major, LK_DEVICE_NAME);

  printk(KERN_INFO "lk: removing device at major %d\n", lk_major);
}

module_init(lk_init)
module_exit(lk_exit)

MODULE_LICENSE("GPL");
