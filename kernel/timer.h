#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"

struct timeval {
  long tv_sec;         /* seconds */
  long tv_usec;        /* microseconds */
};

struct timezone {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime;     /* type of dst correction */
};      

inline static unsigned long long rdtsc(void)
{
  unsigned int lo, hi;

  asm volatile ("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");

  return ((unsigned long long)hi << 32ULL | (unsigned long long)lo);
}

// Funtions
void timer_init(void);
void timer_handler(int irq_no, QWORD rip);

size_t sys_get_ticks(void);
void sys_msleep(unsigned int ms);

int sys_gettimeofday(struct timeval *tv, struct timezone *tz) ;
void gettimeofday_init(void) ;

size_t get_start_tsc(void);
size_t get_freq(void);

#endif  /* __TIMER_H__ */
