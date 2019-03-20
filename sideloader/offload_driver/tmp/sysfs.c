/*
 * =====================================================================================
 *
 *       Filename:  sysfs.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016년 02월 17일 16시 02분 34초
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include "../../include/arch.h"

extern unsigned long g_boot_addr ;
extern unsigned long g_vcon_addr ;
extern unsigned long g_pml_addr ;

static ssize_t boot_addr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t console_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t cpu_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t ipcs_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t pager_memory_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static struct kobj_attribute boot_addr_attribute =
	__ATTR(boot_addr, 0444, boot_addr_show,  NULL ) ;

static struct kobj_attribute console_attribute =
	__ATTR(console, 0444, console_show,  NULL ) ;

static struct kobj_attribute cpu_status_attribute =
	__ATTR(cpu_status, 0444, cpu_status_show,  NULL ) ;

static struct kobj_attribute ipcs_status_attribute =
	__ATTR(ipcs_status, 0444, ipcs_status_show,  NULL ) ;

static struct kobj_attribute pager_memory_status_attribute =
	__ATTR(pager_memory_status, 0444, pager_memory_status_show,  NULL ) ;

static struct attribute *lkernel_attr[] = {
	&boot_addr_attribute.attr,
	&console_attribute.attr,
	&cpu_status_attribute.attr,
	&ipcs_status_attribute.attr,
	&pager_memory_status_attribute.attr,
	NULL,
};

static struct attribute_group lkernel_attr_group = {
	.attrs = lkernel_attr,
};

static struct kobject *lkernel_kobj;

/*
 * show boot address
 */
static ssize_t boot_addr_show(struct kobject *kobj, struct kobj_attribute *attr, char
		*buf)
{
  return sprintf(buf, "boot_addr: %lx\nvcon_addr: %lx\n pml_addr: %lx\n",g_boot_addr, g_vcon_addr, g_pml_addr ) ;
}

/*
 * show vconsole
 */
static ssize_t console_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
  int bytes = -1;
  memcpy(buf, __va(g_vcon_addr), 4096);
  bytes =  4096 + sprintf(buf+4096, "\n" ) ;

  return bytes ;
}

/*
 * show cpu status
 */
static ssize_t cpu_status_show(struct kobject *kobj, struct kobj_attribute
		*attr, char *buf)
{
  int bytes = -1;

  memcpy(buf, __va(g_boot_addr+3840), 220) ;
  bytes = 220 + sprintf( buf+220, "\n") ;
  return bytes;
}

/*
 * show ipc status
 */
static ssize_t ipcs_status_show(struct kobject *kobj, struct kobj_attribute
		*attr, char *buf)
{
  return sprintf(buf, "%d %d %d %d %d %d\n", *((unsigned int*)__va(g_boot_addr+3840+220)),
				*((unsigned int*)__va(g_boot_addr+3840+220+4)),
				*((unsigned int*)__va(g_boot_addr+3840+220+8)),
				*((unsigned int*)__va(g_boot_addr+3840+220+12)),
				*((unsigned int*)__va(g_boot_addr+3840+220+16)),
				*((unsigned int*)__va(g_boot_addr+3840+220+20)) ) ;

}

/*
 * show pager memory status
 */
static ssize_t pager_memory_status_show(struct kobject *kobj, struct kobj_attribute
		*attr, char *buf)
{
  char *addr = NULL;
  int bytes = -1;

  addr = (char*)ioremap(BASE_32, 16*1024) ;
  if (addr == NULL )
  {
    return sprintf(buf,"%d %d\n",  -1, -1 ) ;
  }

  bytes =sprintf(buf, "%ld %ld\n", *((unsigned long*)(addr+4096)), *((unsigned long*)(addr+4096+8))) ;

  iounmap(addr) ;

  return bytes ;
}

/*
 * initialize sysfs
 */
int lkernel_sysfs_init(void)
{
  int retval = 0;

  lkernel_kobj = kobject_create_and_add("lkernel",NULL );

  if (!lkernel_kobj)
    return -ENOMEM;

  retval = sysfs_create_group(lkernel_kobj,&lkernel_attr_group);

  if (retval)
    kobject_put(lkernel_kobj);

  return retval;
}

/*
 * exit sysfs
 */
void lkernel_sysfs_exit(void)
{
  kobject_put(lkernel_kobj);
}
