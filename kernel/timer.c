// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/types.h>
#include "timer.h"
#include "thread.h"
#include "localapic.h"
#include "shellstorage.h"
#include "delay.h"
#include "stat.h"
#include "console.h"

static QWORD g_tick_count[MAX_PROCESSOR_COUNT] = {0, }; 
static QWORD g_tick_total_prev[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_total_curr[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_cpu_prev[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_cpu_curr[MAX_PROCESSOR_COUNT] = {0, };

static unsigned long long start_tsc;
static unsigned long long freq;

/**
 * detect the frequency of the CPU
 */
unsigned long long detect_cpu_frequency()
{
  unsigned long long ts1, ts2;

  ts1 = rdtsc();
  az_mdelay(50);
  ts2 = rdtsc();

  return ((ts2-ts1) * 20);
}

/*
 * initialize timer
 */
void timer_init(void)
{
  int i = 0;

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++)
    g_tick_count[i] = 0;

  // initialize tick counter
  freq = detect_cpu_frequency();
  start_tsc = rdtsc();
}

/**
 * timer interrupt handler
 */
void timer_handler(int irq_no, QWORD rip)
{
  int cid, pid;
  TCB *curr = get_current();

  lapic_send_eoi();

  cid = (int) get_apic_id();
  pid = (int) get_papic_id();

#ifdef DEBUG
  lk_print_xy(0, 22, "TIMER: core id: %d, irq_no: %d", cid, irq_no);
#endif

#if 0
  if (cid >= 20 && cid < 28) {
    lk_print_xy( (cid%4)*15, (cid/4)+12, "%d, %p", cid, g_tick_count2[cid]);
  }
#endif

  g_tick_count[cid]++;

  // Calculate load of the cpu at 100 ticks intervals
  g_tick_total_curr[cid]++;
  if (!(curr->id < MAX_PROCESSOR_COUNT))
    g_tick_cpu_curr[cid]++;

  if (g_tick_count[cid] % 100 == 0) {
    QWORD cpu_load = (g_tick_cpu_curr[cid]-g_tick_cpu_prev[cid]) * 100 / 
                     (g_tick_total_curr[cid] - g_tick_total_prev[cid]);
    set_cpu_load(pid, cpu_load);

    g_tick_cpu_prev[cid] = g_tick_cpu_curr[cid];
    g_tick_total_prev[cid] = g_tick_total_curr[cid];
  }

  store_timer_info(cid, g_tick_count[cid]);

  // Expect one-shot timer expired - (no need lapic_stop_timer)
  decrease_remaining_time_slice(adjust_time_slice());

  // If current thread has remaining_time slice, run this thread again.
  if (curr->remaining_time_slice > 0) {
    lapic_start_timer_oneshot(curr->remaining_time_slice);
  } else {
    schedule(THREAD_INTENTION_READY);
  }
}

size_t sys_get_ticks(void)
{
  return g_tick_count[get_apic_id()];
}

void sys_msleep(unsigned int ms)
{
  // To be implemented
}

/**
 * return the start tsc
 */
size_t get_start_tsc()
{
  return start_tsc;
}

/**
 * return the cpu frequency
 */
size_t get_freq()
{
  return freq;
}
 
int sys_gettimeofday(struct timeval *tv, void *tz)
{
  if (start_tsc == 0)
    start_tsc = get_start_tsc();

  if (freq == 0)
    freq = get_freq();

  if(tv) {
    size_t diff = rdtsc() - start_tsc;
    tv->tv_sec = diff/freq;
    tv->tv_usec = ((diff - tv->tv_sec * freq) * 1000000ULL) / freq;
  }

  return 0;
}

