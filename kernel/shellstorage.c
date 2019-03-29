#include <sys/lock.h>
#include "shellstorage.h"
#include "console.h"
#include "arch.h"

extern QWORD g_memory_start;
static SHELL_STORAGE_AREA *g_ss_area;

static unsigned int *log_front, *log_rear ;
static char *log_buffer ;

static spinlock_t log_lock ;

/**
 * Initialize Shell Storage Area 
 */
void shell_storage_area_init(void)
{
  spinlock_init(&log_lock) ;

  log_buffer = (char*) va(CONFIG_SHELL_STORAGE_AREA+LOG_LENGTH);
  log_front = (unsigned int *) va(CONFIG_SHELL_STORAGE_AREA);
  log_rear = (unsigned int *) va(CONFIG_SHELL_STORAGE_AREA+4);    

  lk_memset(log_buffer, 0, (MAX_LOG_COUNT+1) * LOG_LENGTH) ;
	
  g_ss_area = (SHELL_STORAGE_AREA *) (va(CONFIG_SHELL_STORAGE_AREA+(5<<20)));
  lk_memset(g_ss_area, 0, sizeof(SHELL_STORAGE_AREA));
}

void shell_enqueue(const char * msg) 
{
#if 1
  spinlock_lock(&log_lock) ;
  if ( *log_rear >= MAX_LOG_COUNT ) return ; 
  lk_memcpy(log_buffer+((*log_rear)*64), msg, lk_strlen(msg)) ; 
  (*log_rear)++ ;
  spinlock_unlock(&log_lock) ;
#endif
}

/**
 * Store pagefault information in SHELL_STORAGE_AREA
 */
void store_pagefault_info(QWORD tid, QWORD fault_address, QWORD error_code, QWORD rip)
{
  PF_AREA *pf_area = &g_ss_area->pf_area;
  PF_INFO *pf_info = NULL;
  DWORD pos = 0;

#if 0
  pos = pf_area->pf_count % MAX_PAGEFAULT_SIZE;
#else
  pos = pf_area->pf_count;
  if (pos > MAX_PAGEFAULT_SIZE)
    return;
#endif
  pf_info = &pf_area->info[pos];

  // store the page fault information
  pf_info->thread_id = tid;
  pf_info->fault_addr = fault_address;
  pf_info->error_code = error_code;
  pf_info->rip = rip;

  pf_area->pf_count++;
}

/**
 * Store thread(tcb) information in SHELL_STORAGE_AREA
 */
void store_tcb_info(QWORD tid, TCB* idle_thread_list)
{
  g_ss_area->thread_area[tid] = idle_thread_list;
}

/**
 * Store timer information in SHELL_STORAGE_AREA
 */
void store_timer_info(int cid, QWORD tick_count)
{
  g_ss_area->timer[cid] = tick_count;
}

/**
 * Store intialize state information in SHELL_STORAGE_AREA
 */
void store_init_stat(int stat)
{
  g_ss_area->init_stat |= stat;
}
