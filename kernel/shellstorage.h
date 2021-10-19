// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __SHELLSTORAGE_H__
#define __SHELLSTORAGE_H__

#include "memory_config.h"
#include "page.h"
#include "thread.h"
#include "az_types.h"

#define	MAX_PAGEFAULT_SIZE	      1024

// init state
#define INIT_IA32E_START_STAT         0x01
#define INIT_GDT_SWITCH_IA32E_STAT    (0x01 << 1)
#define INIT_IDT_STAT                 (0x01 << 2)
#define INIT_PAGETABLE_STAT	      (0x01 << 3)
#define INIT_SYSTEMCALL_STAT	      (0x01 << 4)
#define INIT_MEMORY_STAT	      (0x01 << 5)
#define INIT_SCHEDULER_STAT	      (0x01 << 6)
#define INIT_TASK_STAT		      (0x01 << 7)
#define INIT_ADDRESSSPACE_STAT	      (0x01 << 8)
#define INIT_INTERRUPT_STAT	      (0x01 << 9)
#define INIT_TIMER_STAT		      (0x01 << 10)
#define INIT_APS_STAT		      (0x01 << 11)
#define INIT_LOAD_TSS_STAT	      (0x01 << 12)
#define INIT_LAPIC_STAT		      (0x01 << 13)
#define INIT_LOADER_STAT	      (0x01 << 14)
#define INIT_IDLE_THREAD_STAT	      (0x01 << 15)

#pragma pack(push, 1)

typedef struct {
  QWORD thread_id;
  QWORD fault_addr;
  QWORD error_code;
  QWORD rip;
} PF_INFO;

typedef struct {
  QWORD pf_count;
  PF_INFO info[MAX_PAGEFAULT_SIZE];
} PF_AREA;

typedef struct {
  QWORD init_stat;
  QWORD timer[MAX_PROCESSOR_COUNT];
  PF_AREA pf_area;
  TCB *thread_area[MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD];
} SHELL_STORAGE_AREA;

#pragma pack(pop)

void shell_storage_area_init(void);
void store_pagefault_info(QWORD tid, QWORD fault_address, QWORD error_code, QWORD rip);
void store_tcb_info(QWORD tid, TCB* idle_thread_list);
void store_timer_info(int cid, QWORD tick_count);
void store_init_stat(int stat);
void shell_enqueue(const char *) ;

#endif  /* __SHELLSTORAGE_H__ */
