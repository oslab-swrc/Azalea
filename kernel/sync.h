#ifndef __SYNC_H__
#define __SYNC_H__

#include "assemblyutility.h"
#include "memory_config.h"
#include "multiprocessor.h"
#include "types.h"
#include "errno.h"
#include "atomic.h"
#include "macros.h"
#include "irq.h"

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

#define rfence() {}
#define sfence() {}
//#define mfence() { asm volatile ("mfence" ::: "memory"); }
#define mfence() {}

// Irqsave spinlock initialization
#define SPINLOCK_IRQSAVE_INIT { ATOMIC_INIT(0), ATOMIC_INIT(1), (DWORD)-1, 0, 0}

typedef struct spinlock_struct {
  volatile unsigned long lock;
} spinlock_t __attribute__ ((aligned (8)));

typedef struct spinlock_irqsave {
  atomic64_t queue;	// Internal queue
  atomic64_t dequeue;	// Internel dequeue
  DWORD cid;		// Core Id of the lock owner
  DWORD cnt;		// Internal counter variable
  BYTE flags;		// Interrupt flag
} spinlock_irqsave_t __attribute__ ((aligned (8)));

// Functions

static inline void spinlock_init(spinlock_t *lock)
{
  lock->lock = FALSE;
}

static inline BOOL spinlock_trylock(spinlock_t *lock)
{
  if (cmpxchg(&(lock->lock), 0, 1) != FALSE)
    return FALSE;
  return TRUE;
}

static inline void spinlock_lock(spinlock_t *lock)
{
  do {
    while (lock->lock == TRUE)
      pause();
  } while (cmpxchg(&(lock->lock), 0, 1) != FALSE);
}

static inline void spinlock_unlock(spinlock_t *lock)
{
  lock->lock = FALSE;
}

/**
 * Initialization of a irqsave spinlock
 * return 0 on success
 *        EINVAL on fail
 */
static inline int spinlock_irqsave_init(spinlock_irqsave_t *lock)
{
  if (likelyval(!lock, 0))
    return -EINVAL;

  atomic64_set(&lock->queue, 0);
  atomic64_set(&lock->dequeue, 1);

  lock->flags = 0;
  lock->cid = (DWORD)-1;
  lock->cnt = 0;

  return 0;
}

/** 
 * Destroy irqsave spinlock after use
 * return 0 on success
 *        EINVAL on fail
 */
static inline int spinlock_irqsave_destroy(spinlock_irqsave_t *lock)
{
  if (likelyval(!lock, 0))
    return -EINVAL;

  lock->flags = 0;
  lock->cid = (DWORD)-1;
  lock->cnt = 0;

  return 0;
}

/** 
 * Lock spinlock on entry of critical section and disable interrupts
 * return 0 on success
 *        EINVAL on fail
 */
inline static int spinlock_irqsave_lock(spinlock_irqsave_t *lock)
{
  QWORD ticket;
  BYTE flags;
  int cid = get_apic_id();

  if (likelyval(!lock, 0))
    return -EINVAL;

  flags = irq_nested_disable();
  if (lock->cid == cid) {
    lock->cnt++;
    return 0;
  }

  ticket = atomic64_inc(&lock->queue);
  while (atomic64_read(&lock->dequeue) != ticket) {
    pause();
  }

  lock->cid = cid;
  lock->flags = flags;
  lock->cnt = 1;

  return 0;
}

/**
 * Unlock spinlock on exit of critical section and re-enable interrupts
 * return 0 on success
 *        EINVAL on fail
 */
inline static int spinlock_irqsave_unlock(spinlock_irqsave_t *lock)
{
  BYTE flags;

  if (likelyval(!lock, 0))
    return -EINVAL;

  lock->cnt--;
  if (!lock->cnt) {
    flags = lock->flags;
    lock->cid = (DWORD) -1;
    lock->flags = 0;

    atomic64_inc(&lock->dequeue);

    irq_nested_enable(flags);
  }

  return 0;
}

static spinlock_irqsave_t malloc_lock = SPINLOCK_IRQSAVE_INIT;

inline static void __sys_malloc_lock(void)
{
  spinlock_irqsave_lock(&malloc_lock);
}

inline static void __sys_malloc_unlock(void)
{
  spinlock_irqsave_unlock(&malloc_lock);
}

static spinlock_irqsave_t env_lock = SPINLOCK_IRQSAVE_INIT;

inline static void __sys_env_lock(void)
{
  spinlock_irqsave_lock(&env_lock);
}

inline static void __sys_env_unlock(void)
{
  spinlock_irqsave_unlock(&env_lock);
}

#endif  /*__SYNC_H__*/
