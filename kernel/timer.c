#include <sys/types.h>
#include "timer.h"
#include "thread.h"
#include "localapic.h"
#include "shellstorage.h"
#include "delay.h"
#include "stat.h"
#include "console.h"
#include "ipocap.h"

static QWORD g_tick_count[MAX_PROCESSOR_COUNT] = {0, }; 
static QWORD g_tick_total_prev[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_total_curr[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_cpu_prev[MAX_PROCESSOR_COUNT] = {0, };
static QWORD g_tick_cpu_curr[MAX_PROCESSOR_COUNT] = {0, };

static unsigned long long start_tsc;
static unsigned long long freq;

#define tsc_to_ms(tsc)    (( tsc ) / (freq / 1000))

typedef enum ipocap_stat {
  RUNNING = 0,
  IDLEING
} CSTAT;

typedef struct ipocap_core_status {
  CSTAT status;
  int  require;  
  // ratio >= 0, <= 100
  BYTE ratio_prev;
  BYTE ratio_curr;
  QWORD quantum_remain;
  QWORD idle_paid;
  QWORD tsc_last;
  DWORD energy_last;
} ipocap_stat;

static ipocap_stat g_core_status[MAX_PROCESSOR_COUNT] = {{0,}, };

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

static inline void inject_idle(DWORD msec) {
  __monitor((void *)CONFIG_SHARED_MEMORY + STAT_START_OFFSET, 0, 0);
  msec = (msec<1) ? 1:msec;
  lapic_start_timer_oneshot(msec);
  __mwait(0x10, MWAITX_ECX_TIMER_ENABLE);
}

#define IPOCAP_PREPAID_RATIO  30

static inline int need_capping (int cid) { 
  QWORD tsc_now, ms_diff;
  QWORD uj_now, uj_limit, uj_static;
  DWORD energy_now;
  int lcid, rcid;
  long cratio_global, cratio_local, cratio_avg;
  int require = get_plimit();;

  if(require == 0) {
    if (g_core_status[cid].require != 0) g_core_status[cid].require = 0;
    if (g_core_status[cid].tsc_last != 0) g_core_status[cid].tsc_last = 0;
    return 0;
  } 
 
  // first, counters has to be initialized.
  if ( unlikely(g_core_status[cid].require == 0 ) ) {
    g_core_status[cid].quantum_remain = 300;
    g_core_status[cid].tsc_last = rdtsc();
    g_core_status[cid].energy_last = rapl_read_energy_counter();
	g_core_status[cid].require = require;
	return 0;
  }

  //read counters
  tsc_now  = rdtsc(); 
  energy_now = rapl_read_energy_counter();
  ms_diff = tsc_to_ms(tsc_now - g_core_status[cid].tsc_last);
 
  uj_now = rapl_energy_to_uj(energy_now - g_core_status[cid].energy_last);
  uj_limit = require * ms_diff;
  uj_static = g_power_base_mw * ms_diff;
  
  // is this system running something? No.
  if (uj_now <= uj_static) {
    g_core_status[cid].ratio_prev = g_core_status[cid].ratio_curr;
    g_core_status[cid].ratio_curr = 0;;
    return g_core_status[cid].ratio_curr;
  }
  
  g_core_status[cid].require = require;
  
  // do we inserting idle aleady?
  if (g_core_status[cid].ratio_curr > 0) { 
	rcid = ((cid+1) < MAX_PROCESSOR_COUNT) ? (cid+1) : 0;
	lcid = ((cid-1) < 0 ) ? (MAX_PROCESSOR_COUNT-1) : (cid-1);

	cratio_avg =  g_core_status[cid].ratio_curr;
	cratio_avg +=  g_core_status[rcid].ratio_curr;
	cratio_avg +=  g_core_status[lcid].ratio_curr;
	
	cratio_avg /= 3;
    
	cratio_global = uj_now - uj_limit;
    cratio_global = (cratio_global*100.0) / ( uj_now - uj_static);
	

	g_core_status[cid].ratio_prev = g_core_status[cid].ratio_curr;
	
//    lk_print_xy(0, cid*2, "[%d] %d %d", cid, uj_now-uj_limit, uj_now-uj_static);
	if (cratio_global < -5) {
	  cratio_local = cratio_avg + cratio_global + 5;
	  g_core_status[cid].ratio_curr = (cratio_local < 0) ? 0: cratio_local;
	}
	else if (cratio_global > 0) {
	  g_core_status[cid].ratio_curr = (int) (cratio_avg + cratio_global);
	}
	else {
	  g_core_status[cid].ratio_curr = cratio_avg;
	}

  }
  else {
   //prepaid: maximum capping ratio at first
    g_core_status[cid].ratio_curr = IPOCAP_PREPAID_RATIO;
  }
  
  if(g_core_status[cid].ratio_curr > 50) {
    g_core_status[cid].ratio_curr = 50;
  } 
 
  g_core_status[cid].tsc_last = tsc_now;
  g_core_status[cid].energy_last = energy_now;

  //lk_print_xy(0, cid*2+1, "ipocap[%d]: %d %d      ", cid, g_core_status[cid].ratio_curr, cratio_global);
  
  return g_core_status[cid].ratio_curr;
}

/**
 * timer interrupt handler
 */
void timer_handler(int irq_no, QWORD rip)
{
  int cid, pid;
  int cratio = 0;
  TCB *curr = get_current();
  unsigned long injecting_ms;
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
  
  if (g_core_status[cid].status == RUNNING) {
    if (!(curr->id < MAX_PROCESSOR_COUNT))
      g_tick_cpu_curr[cid]++;

    if (unlikely(g_tick_count[cid] % 100 == 0)) {
      QWORD cpu_load = (g_tick_cpu_curr[cid]-g_tick_cpu_prev[cid]) * 100 / 
                       (g_tick_total_curr[cid] - g_tick_total_prev[cid]);
      set_cpu_load(pid, cpu_load);

      g_tick_cpu_prev[cid] = g_tick_cpu_curr[cid];
      g_tick_total_prev[cid] = g_tick_total_curr[cid];
    }

    store_timer_info(cid, g_tick_count[cid]);

    // Expect one-shot timer expired - (no need lapic_stop_timer)
    decrease_remaining_time_slice(adjust_time_slice());

    if ( (cratio = need_capping(cid)) > 0 ) {
      g_core_status[cid].status = IDLEING;
	  injecting_ms = ((cratio * THREAD_DEFAULT_TIME_SLICE) / ( 100 - cratio)) + 1;
      //lk_print_xy(0, cid*2+2, "  ->  %d      ", injecting_ms);
      inject_idle(injecting_ms);
    }
  }
  else {
    g_core_status[cid].status=RUNNING;
  }

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

