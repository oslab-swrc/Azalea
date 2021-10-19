// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <errno.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lk/cmds.h"

static char *thread_state_str[] = { "--", "CR", "RU", "RE", "BL", "EX" };
static char *thread_state_str_f[] = { "NOTALLOC", "CREATED", "RUNNING", "READY", "BLOCKED", "EXITED" };

void display_tcb_info(TCB * tcb)
{
  printf("\nid=%ld, stack=0x%lx, user_stack=0x%lx", tcb->id, tcb->stack, tcb->user_stack);
  printf("\nstack_base=0x%lx, flags=0x%lx, running_core=%d", tcb->stack_base, tcb->flags, tcb->running_core);
  printf("\ncore_mask=0x%lx%lx%lx%lx%lx%lx%lx%lx", tcb->core_mask.mask[7],
          tcb->core_mask.mask[6], tcb->core_mask.mask[5],
          tcb->core_mask.mask[4], tcb->core_mask.mask[3],
          tcb->core_mask.mask[2], tcb->core_mask.mask[1],
          tcb->core_mask.mask[0]);
  printf("\nstate=%s, acc_ltc=0x%lx", thread_state_str_f[tcb->state], tcb->acc_ltc);
  printf("\nname=%ld, as=0x%x", tcb->name, 0);
  printf("\ntime_slice=0x%lx, remaining_time_slice=0x%lx", tcb->time_slice, tcb->remaining_time_slice);
  printf("\nintention=0x%x, ref_cnt=0x%x", (tcb->intention).c, (tcb->refc).c);
  printf("\ntime_quantum=0x%lx, remaining_time_quantum=0x%lx", tcb->time_quantum, tcb->remaining_time_quantum);
}

void display_thread_usage()
{
  printf("[usage] : cmd THREAD idle [0~%d]\n", MAX_PROCESSOR_COUNT - 1);
  printf("                     user [%d~%d]\n", MAX_PROCESSOR_COUNT,
         MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD - 1);
}

int main(int argc, char *argv[])
{
  int ret = 0;
  char *buf = NULL;

  if (argc < 2) {
    printf("[usage] : cmd [PF|STAT|THREAD|TIMER]\n");
    return -1;
  }
  
  // PageFault Info
  if (!strcmp(argv[1], "PF")) {
    PF_AREA *pf_area = NULL;
    PF_INFO *pf_info = NULL;
    int fd = -1;
    int i = 0;
    int start = 0, count = 0;

    buf = malloc(sizeof(PF_AREA));
    if (buf == NULL) {
      printf("memory allocation failed \n");
      return -1;
    };

    fd = open("/dev/lk", O_RDONLY);
    if (fd < 0) {
      printf("lk open error\n");
      return -1;
    }

    ret = ioctl(fd, LK_CMD_PAGEFAULT, buf);
    if (ret < 0) {
      printf("ioctl failed \n");
      close(fd);
      free(buf);
      return -1;
    }

    pf_area = (PF_AREA *) buf;

#if 0
    if (pf_area->pf_count > MAX_PAGEFAULT_SIZE) {
      start = pf_area->pf_count % MAX_PAGEFAULT_SIZE;
      count = MAX_PAGEFAULT_SIZE;
    } else {
      start = 0;
      count = pf_area->pf_count;
    }
#endif

    printf("== pf_count: %d ==", pf_area->pf_count);
    printf("\n number  tid   cid  err_code     fault_address             rip \n");
    printf("-----------------------------------------------------------------\n");

    for (i=0; i<pf_area->pf_count; i++) {
      pf_info = &pf_area->info[i];

      printf(" %4d   %4d   %3d  %7lx  %16lx  %16lx\n", i,
             pf_info->thread_id, pf_info->cid, pf_info->error_code,
             pf_info->fault_addr, pf_info->rip);
    }
    printf("\n");

    close(fd);
    free(buf);
  
  } else if (!strcmp(argv[1], "STAT")) {    // Status Info
    QWORD status = 0;
    int fd = 0;

    buf = malloc(sizeof(QWORD));
    if (buf == NULL) {
      printf("memory allocation failed \n");
      return -1;
    };

    fd = open("/dev/lk", O_RDONLY);
    if (fd < 0) {
      printf(" cmds open error\n");
      return -1;
    }

    ret = ioctl(fd, LK_CMD_STAT, buf);
    if (ret < 0) {
      printf("ioctl failed \n");
      close(fd);
      free(buf);
      return -1;
    }

    status = *((QWORD *) buf);
    printf("\nSwitch to IA-32e mode success!!");
    printf("\nIA-32e C language kernel started.............[%s]",
           (status & INIT_IA32E_START_STAT) ? "Pass" : "Fail");
    printf("\nInit GDT and switch to IA-32e mode...........[%s]",
           (status & INIT_GDT_SWITCH_IA32E_STAT) ? "Pass" : "Fail");
    printf("\nInit IDT.....................................[%s]",
           (status & INIT_IDT_STAT) ? "Pass" : "Fail");
    printf("\nInit Kernel Page Tables......................[%s]",
           (status & INIT_PAGETABLE_STAT) ? "Pass" : "Fail");
    printf("\nInit System Calls............................[%s]",
           (status & INIT_SYSTEMCALL_STAT) ? "Pass" : "Fail");
    printf("\nInit Free Memory Management..................[%s]",
           (status & INIT_MEMORY_STAT) ? "Pass" : "Fail");
    printf("\nInit Scheduler...............................[%s]",
           (status & INIT_SCHEDULER_STAT) ? "Pass" : "Fail");
    printf("\nInit Task / Thread...........................[%s]",
           (status & INIT_TASK_STAT) ? "Pass" : "Fail");
    printf("\nInit Address Space...........................[%s]",
           (status & INIT_ADDRESSSPACE_STAT) ? "Pass" : "Fail");
    printf("\nInit Interrupt Handler.......................[%s]",
           (status & INIT_INTERRUPT_STAT) ? "Pass" : "Fail");
    printf("\nInit Timer...................................[%s]",
           (status & INIT_TIMER_STAT) ? "Pass" : "Fail");
    printf("\nStart APs....................................[%s]",
           (status & INIT_APS_STAT) ? "Pass" : "Fail");
    printf("\nLoad TSS.....................................[%s]",
           (status & INIT_LOAD_TSS_STAT) ? "Pass" : "Fail");
    printf("\nEnable Local APIC............................[%s]",
           (status & INIT_LAPIC_STAT) ? "Pass" : "Fail");
    printf("\nLoading Binary Loader........................[%s]",
           (status & INIT_LOADER_STAT) ? "Pass" : "Fail");
    printf("\nRunning Idle Thread..........................[%s]\n",
           (status & INIT_IDLE_THREAD_STAT) ? "Pass" : "Fail");
    printf("\n");

    close(fd);
    free(buf);

  } else if (!strcmp(argv[1], "THREAD")) {    // Status Info
    TCB *tcb_ptr = NULL;
    int fd = 0, tid = 0;
    int i = 0;
    int count = 0;

    if (argc != 3 && argc != 4) {
      display_thread_usage();
      return -1;
    }

    buf = malloc(sizeof(TCB) * (MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD));
    if (buf == NULL) {
      printf("memory allocation failed\n");
      return -1;
    };

    fd = open("/dev/lk", O_RDONLY);
    if (fd < 0) {
      printf("cmds open error\n");
      return -1;
    }

    ret = ioctl(fd, LK_CMD_THREAD, buf);
    if (ret < 0) {
      printf("ioctl failed \n");
      close(fd);
      free(buf);
      return -1;
    }

    tcb_ptr = (TCB *) buf;

    if (!memcmp(argv[2], "idle", strlen(argv[2]))) {     // IDLE THREAD
      if (argc == 3) {
        for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
          int rts = tcb_ptr->remaining_time_slice < 0 ? 0 : tcb_ptr->remaining_time_slice;
          int rcore = (tcb_ptr->running_core == 65535) ? -1 : tcb_ptr->running_core;
          if (!(i % 8))
            printf("\n%3d->  %5s[%2lx][%lx]", i, thread_state_str[tcb_ptr->state], rcore, rts);
          else
            printf("  %5s[%2lx][%lx]", thread_state_str[tcb_ptr->state], rcore, rts);

          tcb_ptr++;
        }                         // for end
        printf("\n");
      } else {
        tid = atoi(argv[3]);
        if (tid < 0 || tid >= MAX_PROCESSOR_COUNT) {
          display_thread_usage();
          return -1;
        }
        tcb_ptr += tid;

        display_tcb_info(tcb_ptr);
      }
    } else if (!memcmp(argv[2], "user", strlen(argv[2]))) {     // USER THREAD
      if (argc == 3) {
        tcb_ptr += MAX_PROCESSOR_COUNT;

        for (i = MAX_PROCESSOR_COUNT, count = 0; i < (MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD); i++, count++) {
          int rts = tcb_ptr->remaining_time_slice < 0 ? 0 : tcb_ptr->remaining_time_slice;
          int rcore = (tcb_ptr->running_core == 65535) ? -1 : tcb_ptr->running_core;
          if (!(count % 8))
            printf("\n%3d->  %5s[%3d][%lx]", i, thread_state_str[tcb_ptr->state], rcore, rts);
          else
            printf("  %5s[%3d][%lx]", thread_state_str[tcb_ptr->state], rcore, rts);

          tcb_ptr++;
        }                         // for end
        printf("\n");
      } else {
        tid = atoi(argv[3]);
        if (tid < MAX_PROCESSOR_COUNT
            || tid >= (MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD)) {
          display_thread_usage();
          return -1;
        }

        tcb_ptr += tid;

        display_tcb_info(tcb_ptr);
      }
    } else
      display_thread_usage();

    printf("\n");

    close(fd);
    free(buf);

  } else if (!strcmp(argv[1], "TIMER")) {    // Status Info
    QWORD *timer = NULL;
    int fd = -1;
    int i = 0;

    buf = malloc(sizeof(QWORD) * MAX_PROCESSOR_COUNT);
    if (buf == NULL) {
      printf("memory allocation failed \n");
      return -1;
    };

    fd = open("/dev/lk", O_RDONLY);
    if (fd < 0) {
      printf(" cmds open error\n");
      return -1;
    }

    ret = ioctl(fd, LK_CMD_TIMER, buf);
    if (ret < 0) {
      printf("ioctl failed \n");
      close(fd);
      free(buf);
      return -1;
    }

    timer = (QWORD *) buf;

    for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
      if (!(i % 8))
        printf("\n%3d->  %lx", i, timer[i]);
      else
        printf("  %lx", timer[i]);
    }
    printf("\n\n");

    close(fd);
    free(buf);

  } else {
    printf("[usage] : cmd [PF|STAT|THREAD|TIMER]\n");
    return -1;
  }

  return 0;
}

// EOF
