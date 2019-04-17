#include <sys/lock.h>
#include "interrupthandler.h"
#include "console.h"
#include "debug.h"
#include "map.h"
#include "memory.h"
#include "shellstorage.h"
#include "thread.h"

#define INTR_CNT 256

extern unsigned int shutdown_kernel ;
extern QWORD g_memory_start;


unsigned int general_fault_exeception_cnt ;

struct intr_handler {
  QWORD user_driver_id;
  QWORD kthread_id;
};

static struct intr_handler g_intr_handler_threads[INTR_CNT];

/*
 * initialize interrupt handler thread
 */
void intr_handler_thread_init(void)
{
  int i = 0;

  for (i = 0; i < INTR_CNT; i++)
    g_intr_handler_threads[i].kthread_id = 0;
}

/* 
 * send end of interrupt
 */
void send_eoi(int irq_no)
{
  lapic_send_eoi();
}

/**
 * Common Exception Handler
 */
void lk_common_exception_handler(int vector_number, QWORD error_code, QWORD v)
{
  lk_print_xy(0, 24, "Exception: %Q %Q %Q %Q", get_apic_id(), (QWORD) vector_number, v, error_code);

  while(1)
    ;
}

/**
 * Common Interrupt Handler
 */
void lk_common_interrupt_handler(int vector_number)
{
  send_eoi(vector_number);
}

/**
 * General Protection Handler
 */
void lk_general_protection_handler(int vector_number, QWORD error_code, QWORD v)
{
	general_fault_exeception_cnt ++ ;
}

/*
     page fault error code:
        bit 0 == 0: no page found       1: protection fault
        bit 1 == 0: read access         1: write access
        bit 2 == 0: kernel mode access  1: user-mode access
        bit 3     : RSVD
        bit 4 ==                        1: fault was an instruction fetch
    ref: Intel manual, Figure 6-9. Page-Fault Error Code
*/
void pagefault_handler(QWORD fault_address, QWORD error_code, QWORD rip)
{
  TCB *current = get_current();
  char *new_page = NULL;
  int ret = 0;

#if 1
{
  int cid = get_apic_id();
  static atomic_t pfcnt;

  lk_print_xy(20, 17, "============= Page Fault Info. =============");
  lk_print_xy(20, (pfcnt.c % 4) + 18, "PageFault[%d]: %d, %Q, %Q, %Q, %d", cid, current->id, fault_address, error_code, rip, pfcnt.c);
  atomic_inc(&pfcnt);
}
#endif
  store_pagefault_info(current->id, fault_address, error_code, rip);
  
  // alloc new page
  new_page = (char *) az_alloc(PAGE_SIZE_4K);

  if( new_page == (char *) NULL)
    debug_halt((char *) __func__, __LINE__);


  // map virtual(fault) address-newpage
  ret = lk_map((QWORD) fault_address, (QWORD) pa(new_page), PAGE_SIZE_4K, 0);
  if(ret == -1)
    debug_halt((char *) __func__, __LINE__);
}

void ipi_handler(int irq_no, QWORD rip)
{
//To Do:

  send_eoi(irq_no) ;

  if ( irq_no == 49 )
	shutdown_kernel = 1 ;
  else
	printk("ipi recieved %d %d\n", irq_no, get_apic_id()) ;

/*
  disable_software_local_apic() ;

  printk("TURN OFF : %d", get_apic_id());

  for (;;)
	{
	__asm__ __volatile__("hlt") ;
	}
*/
  send_eoi(irq_no);
}

/*
 * regist interrupt
 */
int register_interrupt(int irq)
{
#if 0
  TCB *kth;

  switch (irq) {

    default: 
      break;
  }

  //debug_halt(__func__, get_current()->id);

  return g_intr_handler_threads[irq].kthread_id;
#else
  return 0;
#endif
}

/*
 * unregist interrupt
 */
int unregister_interrupt(int irq)
{
//  get_tcb(g_intr_handler_threads[irq].kthread_id)->state = THREAD_STATE_BLOCKED;
  g_intr_handler_threads[irq].kthread_id = (QWORD) NULL;
  g_intr_handler_threads[irq].user_driver_id = (QWORD) NULL;

  return 0;
}
