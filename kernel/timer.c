#include "timer.h"
#include "thread.h"
#include "localapic.h"
#include "shellstorage.h"

static QWORD g_tick_count[MAX_PROCESSOR_COUNT] = {0, }; 

/*
 * initialize timer
 */
void timer_init(void)
{
  int i = 0;

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++)
    g_tick_count[i] = 0;
}

/**
 * timer interrupt handler
 */
void timer_handler(int irq_no, QWORD rip)
{
  int cid = 0;
  TCB *curr = get_current();

  lapic_send_eoi();

  cid = (int) get_apic_id();

#ifdef DEBUG
  lk_print_xy(0, 22, "TIMER: core id: %d, irq_no: %d", cid, irq_no);
#endif

#if 0
  if (cid >= 20 && cid < 28) {
    lk_print_xy( (cid%4)*15, (cid/4)+12, "%d, %p", cid, g_tick_count2[cid]);
  }
#endif

  g_tick_count[cid]++;

#ifdef DEBUG
  lk_print_xy(0, 23, "Tick_count: %Q %Q %Q %Q %Q %Q %Q %Q",
           g_tick_count[0],
           g_tick_count[1],
           g_tick_count[2],
           g_tick_count[3],
           g_tick_count[4],
           g_tick_count[5],
           g_tick_count[6],
           g_tick_count[7]);
#endif

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
