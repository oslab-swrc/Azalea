#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>

#include "atomic.h"
#include "types.h"

/* Macro */
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))

// __old and __new is for 64bit register assignement
#define cmpxchg(ptr, old, new) ({ \
  __typeof__(*(ptr)) __ret; \
  __typeof__(*(ptr)) __old = old; \
  __typeof__(*(ptr)) __new = new; \
  volatile unsigned char *__ptr; \
  __ptr = (volatile unsigned char *)(ptr); \
  asm volatile("lock; cmpxchgq %2,%1\n\t" \
    : "=a" (__ret), "+m" (*__ptr) \
    : "r" (__new), "0" (__old) \
    : "memory"); \
  __ret; \
})

// Measure functions
#define tsc_start(start) \
  asm volatile("cpuid\n\t" \
         "rdtsc\n\t" \
         "shlq $32, %%rdx\n\t" \
         "orq %%rdx, %%rax\n\t" \
         "movq %%rax, %0":\
         "=r"(start): : "rax", "rbx", "rdx", "rcx")

#define tsc_end(end) \
  asm volatile("rdtsc\n\t" \
         "shlq $32, %%rdx\n\t" \
         "orq %%rdx, %%rax\n\t" \
         "movq %%rax, %0\n\t" \
         "cpuid":\
         "=r"(end): : "rax", "rbx", "rdx", "rcx")

struct az_timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* microseconds */
};

struct az_timezone {
  int     tz_minuteswest; /* minutes west of Greenwich */
  int     tz_dsttime;     /* type of dst correction */
};

inline static unsigned long long rdtsc(void)
{
  unsigned int lo, hi;

  asm volatile ("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");

  return ((unsigned long long)hi << 32ULL | (unsigned long long)lo);
}

/* Functions */
int a_memcpy(void *destination, const void *source, int size);
int a_strlen(const char *buffer);

int _itoa(long value, char *buffer, int radix);
void int_to_str(int n, char *str);
int hextostring(QWORD value, char *buffer);
int decimaltostring(long value, char *buffer);
void reversestring(char *buffer);

int vsprintf(char *buffer, const char *format_string, va_list ap);
int print_xy(int x, int y, const char *buffer, ...);
int kprintf(const char *fmt, ...);
int print( const char *buffer, ...);

int az_gettimeofday(struct az_timeval *tv, struct az_timezone *tz);

void srand(unsigned int seed);
int rand();

#endif /*__UTILITY_H__*/
