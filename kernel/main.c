// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/lock.h>
#include "assemblyutility.h"
#include "console.h"
#include "debug.h"
#include "descriptor.h"
#include "interrupthandler.h"
#include "localapic.h"
#include "map.h"
#include "memory_config.h"
#include "multiprocessor.h"
#include "offload_mmap.h"
#include "page.h"
#include "shellstorage.h"
#include "sync.h"
#include "systemcall.h"
#include "thread.h"
#include "az_types.h"
#include "utility.h"
#include "memory.h"
#include "timer.h"
#include "stat.h"
#include "console_function.h"
#include "console_mmap.h"

#define	OFFLOAD_ENABLE

static void main_for_ap(void);
BOOL start_ap(void);

extern QWORD g_vcon_addr;
extern int g_ap_ready;
extern int g_cpu_start;
extern int g_cpu_end;
extern int g_cpu_size;
extern int g_ukid;
extern QWORD g_memory_start;
extern QWORD g_memory_end;
extern QWORD g_shared_memory;

inline static void flush_cache(void)
{
  asm volatile ("wbinvd" ::: "memory");
}

void HALT()
{
  disable_interrupt();
  flush_cache() ;

  for (;;) {
    hlt();
  }
}

/**
 * @brief IA-32e mode main
 * @param boot_mode - 0:AP, 1:BST
 */
void Main(int boot_mode)
{
  int xloc = 46, yloc = 0;

  g_ukid = (*((int*) CONFIG_UKID_ADDR));
  g_vcon_addr = CONFIG_SHARED_MEMORY + VCON_START_OFFSET + (g_ukid * PAGE_SIZE_4K);
  g_cpu_start = (*((QWORD*) CONFIG_CPU_START));
  g_cpu_end = (*((QWORD*) CONFIG_CPU_END));
  g_cpu_size = g_cpu_start;
  g_memory_start = (*(QWORD*) (CONFIG_MEM_START + CONFIG_PAGE_OFFSET)) << 30;
  g_memory_end = (*(QWORD*) (CONFIG_MEM_END + CONFIG_PAGE_OFFSET)) << 30;
  g_shared_memory = ((QWORD) (UNIKERNEL_START-SHARED_MEMORY_SIZE)) << 30;

  if (boot_mode == 0) { // AP mode
    while(g_ap_ready == 0)
      pause();
    main_for_ap();
  }

  kernel_pagetables_init(CONFIG_KERNEL_PAGETABLE_ADDRESS);

  lk_print_xy(0, yloc++, "IA-32e C language kernel started.............[Pass]");
  lk_print_xy(0, yloc++, "(ID: %d, VCON: 0x%q)", g_ukid, g_vcon_addr);
  lk_print_xy(0, yloc++, "(CPU_NUM: %d)", g_cpu_start);
  lk_print_xy(0, yloc++, "(MEMORY_START: %d GB,MEMORY_END: %d GB)", g_memory_start>>30, g_memory_end>>30);
  lk_print_xy(0, yloc++, "Init Kernel Page Tables .....................[Pass]");
  store_init_stat(INIT_IA32E_START_STAT);
  store_init_stat(INIT_PAGETABLE_STAT);

  lk_print_xy(0, yloc, "Init STAT Memory ............................[    ]");
  stat_init();
  lk_print_xy(xloc, yloc++, "Pass");

#if DEBUG
  lk_print_xy(0, yloc, "Memory check.................................[    ]");
  if (az_check_memory() == FALSE) {
    lk_print_xy(xloc, yloc++, "Fail");
    debug_halt((char *)__func__, __LINE__);
  }
  lk_print_xy(xloc, yloc++, "Pass");
#endif

  lk_print_xy(0, yloc, "Init Free Memory Management .................[    ]");
  free_mem_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_MEMORY_STAT);

  lk_print_xy(0, yloc, "Init Shell storage ..........................[    ]");
  shell_storage_area_init();
  lk_print_xy(xloc, yloc++, "Pass");

  lk_print_xy(0, yloc, "Init GDT and switch to IA-32e mode...........[    ]");
  gdt_and_tss_init();
  load_gdtr(va(GDTR_START_ADDRESS));
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_GDT_SWITCH_IA32E_STAT);

  lk_print_xy(0, yloc, "Load TSS ....................................[    ]");
  load_tr(GDT_TSS); 
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_LOAD_TSS_STAT);

  lk_print_xy(0, yloc, "Init IDT ....................................[    ]");
  idt_init();
  load_idtr(va(IDTR_START_ADDRESS));
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_IDT_STAT);

  lk_print_xy(0, yloc, "Init System Calls ...........................[    ]");
  systemcall_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_SYSTEMCALL_STAT);

  lk_print_xy(0, yloc, "Init Scheduler ..............................[    ]");
  sched_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_SCHEDULER_STAT);

  lk_print_xy(0, yloc, "Init Thread .................................[    ]");
  thread_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_TASK_STAT);

  lk_print_xy(0, yloc, "Init Map ....................................[    ]");
  map_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_ADDRESSSPACE_STAT);

  lk_print_xy(0, yloc, "Init Interrupt Handler ......................[    ]");
  intr_handler_thread_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_INTERRUPT_STAT);

  lk_print_xy(0, yloc, "Init Timer ..................................[    ]");
  timer_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_TIMER_STAT);

  lk_print_xy(0, yloc, "Start APs ...................................[    ]");
  if ( start_ap() == FALSE ) {
    lk_print_xy(xloc, yloc++, "Fail");
    debug_halt((char *)__func__, __LINE__);
  }
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_APS_STAT);

  lk_print_xy(0, yloc, "Enable Local APIC............................[    ]");
  enable_software_local_apic();
  set_task_priority(0);
  local_vector_table_init();
  lk_print_xy(xloc, yloc++, "Pass");
  store_init_stat(INIT_LAPIC_STAT);

  lk_print_xy(0, yloc, "Init Signal .................................[    ]");
  signal_init();
  lk_print_xy(xloc, yloc++, "Pass");

  lk_print_xy(0, yloc, "Remove low identical mapping.................[    ]");
  remove_low_identical_mapping(CONFIG_KERNEL_PAGETABLE_ADDRESS);
  lk_print_xy(xloc, yloc++, "Pass");

#ifdef	OFFLOAD_ENABLE
  // init offload console channel
  if(init_console_channel() == TRUE) {
    cs_boot_msg_print(yloc);

    lk_print_xy(0, yloc++, "Init Console ................................[Pass]");
    cs_puts("Init Console ................................[Pass]\n");

    // init offload channel
    lk_print_xy(0, yloc, "Init IO Offload .............................[    ]");
    cs_puts("Init IO Offload .............................[    ]");
    if(init_offload_channel() == TRUE) {
      lk_print_xy(xloc, yloc++, "Pass");
      cs_puts("\b\b\b\b\bPass\n");
    }
    else {
      lk_print_xy(xloc, yloc++, "Fail");
      cs_puts("\b\b\b\b\bFail\n");
    }

    cs_printf("\n> Azalea unikernel(ID: %d) successfully launched!!!\n", g_ukid);
    cs_puts("> ");
  }
  else {
    lk_print_xy(0, yloc++, "Init Console ................................[Fail]");

    // init offload channel
    lk_print_xy(0, yloc, "Init IO Offload .............................[    ]");
    if(init_offload_channel() == TRUE) {
      lk_print_xy(xloc, yloc++, "Pass");
    }
    else {
      lk_print_xy(xloc, yloc++, "Fail");
    }
  }
#endif

  // Start idle thread
  store_init_stat(INIT_IDLE_THREAD_STAT);
  start_idle_thread(THREAD_TYPE_BSP);
}

/**
 * @brief start APs
 * return success (TRUE), fail (FALSE)
 */
BOOL start_ap(void)
{
  if (startup_ap() == FALSE)
    return FALSE;

  g_ap_ready = 1;

  return TRUE;
}

/**
 * @brief main function for APs
 * return none
 */
static void main_for_ap(void)
{
  // Init GDT
  load_gdtr(va(GDTR_START_ADDRESS));

  // Init TSS
  load_tr(GDT_TSS + (get_apic_id() * sizeof(GDT_ENTRY16)));

  // Init IDT
  load_idtr(va(IDTR_START_ADDRESS));

  // Enabling local APIC
  enable_software_local_apic();
  set_task_priority(0);
  local_vector_table_init();

  // Init Systemcall
  systemcall_init();

  start_idle_thread(THREAD_TYPE_AP);
}
