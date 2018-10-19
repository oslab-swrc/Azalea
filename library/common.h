#ifndef __COMMON_H__
#define __COMMON_H__

#include "atomic.h"

#define MAX_NUM_THREAD		1024

#define tid_t               int
#define signal_handler_t    int

#if 0
typedef struct spinlock_struct {
  volatile unsigned long lock;
} spinlock_t __attribute__ ((aligned (8)));
#endif

typedef struct spinlock_irqsave {
  atomic64_t queue;     // Internal queue
  atomic64_t dequeue;   // Internel dequeue
  DWORD cid;            // Core Id of the lock owner
  DWORD cnt;            // Internal counter variable
  BYTE flags;           // Interrupt flag
} spinlock_irqsave_t;

typedef struct sem {
  DWORD value;                  // Resource available count
  DWORD queue[MAX_NUM_THREAD];  // Queue of waiting threads
  DWORD wpos;                   // Position in queue to add a thread
  DWORD rpos;                   // Position in queue to get a thread
  spinlock_irqsave_t lock;      // Access lock
} sem_t;

#endif  /* __COMON_H__ */
