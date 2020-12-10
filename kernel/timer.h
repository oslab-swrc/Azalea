#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/types.h>
#include "az_types.h"

static inline QWORD rdtsc(void)
{
  DWORD lo, hi;

  asm volatile ("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");

  return (QWORD) ( lo | ((QWORD) hi)  << 32);
}

// Funtions
void timer_init(void);
void timer_handler(int irq_no, QWORD rip);

size_t sys_get_ticks(void);
void sys_msleep(unsigned int ms);

//int sys_gettimeofday(struct timeval *tv, struct timezone *tz) ;
int sys_gettimeofday(struct timeval *tv, void *tz);
void gettimeofday_init(void) ;

size_t get_start_tsc(void);
size_t get_freq(void);

#endif  /* __TIMER_H__ */
