#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

/* dummy lock routines for single-threaded aps */

typedef int _LOCK_T;
typedef int _LOCK_RECURSIVE_T;

#include <_ansi.h>
#if 1
#define __LOCK_INIT(class,lock) static spinlock_t lock = SPINLOCK_INIT
#define __LOCK_INIT_RECURSIVE(class,lock) static spinlock_t lock = SPINLOCK_INIT
#else
#define __LOCK_INIT(class,lock) static int lock = 0
#define __LOCK_INIT_RECURSIVE(class,lock) static int lock = 0
#endif
#define __lock_init(lock) (_CAST_VOID 0)
#define __lock_init_recursive(lock) (_CAST_VOID 0)
#define __lock_close(lock) (_CAST_VOID 0)
#define __lock_close_recursive(lock) (_CAST_VOID 0)
#define __lock_acquire(lock) spinlock_lock(&lock)
#define __lock_acquire_recursive(lock) spinlock_lock(&lock)
#define __lock_try_acquire(lock) (_CAST_VOID 0)
#define __lock_try_acquire_recursive(lock) (_CAST_VOID 0)
#define __lock_release(lock) spinlock_unlock(&lock)
#define __lock_release_recursive(lock) spinlock_unlock(&lock)


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

#define TRUE   1
#define FALSE  0
#define BOOL   unsigned char

#define rfence() {}
#define sfence() {}

#if 0
static inline void spinlock_init(int *lock)
{
  if (cmpxchg(&lock, 0, 1) != FALSE)
    return FALSE;
  return TRUE;
}

static inline void spinlock_lock(int *lock)
{
  do {
    while (lock == TRUE)
      __asm volatile ("pause" ::: "memory"); 
  } while (cmpxchg(&lock, 0, 1) != FALSE);
}

static inline void spinlock_unlock(int *lock)
{
  lock = FALSE;
}
#else
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
#endif

#endif /* __SYS_LOCK_H__ */

