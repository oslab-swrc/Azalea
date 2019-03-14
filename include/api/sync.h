#ifndef __SYNC_H__
#define __SYNC_H__

#define TRUE   1
#define FALSE  0
#define BOOL   unsigned char

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

// Irqsave spinlock initialization
#define SPINLOCK_INIT { 0 }

typedef struct spinlock_struct {
  volatile unsigned long lock;
} spinlock_t __attribute__ ((aligned (8)));

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
      __asm volatile ("pause" ::: "memory");
  } while (cmpxchg(&(lock->lock), 0, 1) != FALSE);
}

static inline void spinlock_unlock(spinlock_t *lock)
{
  lock->lock = FALSE;
}

#endif  /*__SYNC_H__*/
