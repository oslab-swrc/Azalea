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

#define OFFLOAD_DEVICE_NAME	"offload"

static int offload_major = 0;
static struct class *offload_class;

static int offload_open(struct inode *inode, struct file *filp);
static int offload_release(struct inode *inode, struct file *filep);
static long offload_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int offload_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations offload_fops = {
  .open = offload_open,
  .release = offload_release,
  .unlocked_ioctl = offload_ioctl,
  .mmap = offload_mmap,
};

/*
 * open offload 
 */
static int offload_open(struct inode *inode, struct file *filp)
{
  return 0;
}

/*
 * relase offload 
 */
static int offload_release(struct inode *inode, struct file *filep)
{
  return 0;
}

/*
 * ioctl offload 
 */
static long offload_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
 return 0;
}

/*
 * mmap offload 
 */
static int offload_mmap(struct file *filp, struct vm_area_struct *vma)
{
  unsigned long vma_size = vma->vm_end - vma->vm_start;
  unsigned long long offset = vma->vm_pgoff << PAGE_SHIFT;

  vma->vm_flags |= VM_IO;  
  vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);

#if 0
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 
#else
  vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#endif

  if(remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, vma_size, vma->vm_page_prot) < 0) {
    printk("offload remap failed\n");
    return -EAGAIN;
  }

  return 0;
}

/*
 * mmap offload 
static unsigned long offload_munmap(void *addr, size_t len)
{
  if (munmap (addr, len) == −1)
    printk("offload munmap failed\n");
}
 */

/*
 * initialize offload
 */
static int __init offload_init(void)
{
  struct device *err_dev = NULL;

  offload_major = register_chrdev(0, OFFLOAD_DEVICE_NAME, &offload_fops);
  if (offload_major < 0) {
    printk(KERN_ERR "unable to register offload : %d\n", offload_major);
    return -1;
  }

  offload_class = class_create(THIS_MODULE, OFFLOAD_DEVICE_NAME);
  err_dev = device_create(offload_class, NULL, MKDEV(offload_major, 0), NULL,
                          OFFLOAD_DEVICE_NAME);

  printk(KERN_INFO "offload: register device at major %d\n", offload_major);

  return 0;
}

/*
 * exitoffload 
 */
void offload_exit(void)
{
  device_destroy(offload_class, MKDEV(offload_major, 0));
  class_unregister(offload_class);
  class_destroy(offload_class);
  unregister_chrdev(offload_major, OFFLOAD_DEVICE_NAME);

  printk(KERN_INFO "offload: removing device at major %d\n", offload_major);
}

module_init(offload_init)
module_exit(offload_exit)

MODULE_LICENSE("GPL");
