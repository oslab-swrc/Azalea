// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "az_types.h"
#include "thread.h"
#include "sync.h"
#include "utility.h"
#include "errno.h"
#include "timer.h"
#include "console.h"
#include "memory.h"

#define	MAX_THREAD_ID		(MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD)
#define	MAX_SEM_QUEUE_SIZE	(MAX_THREAD_ID)
// Macros
#define SEM_INIT(v) {v, {[0 ... MAX_NUM_THREAD-1] = MAX_NUM_THREAD}, 0, SPINLOCK_IRQSAVE_INIT}

typedef struct sem {
  DWORD value;				// Resource available count
  DWORD *queue;				// Queue of waiting threads
  DWORD wpos;				// Position in queue to add a thread
  DWORD rpos;				// Position in queue to get a thread
  spinlock_irqsave_t lock;		// Access lock
} sem_t;

// Functions
static inline int az_sem_init(sem_t* s, DWORD v)
{
  DWORD i;

  if (likelyval(!s, 0))
    return -EINVAL;

  s->queue = (DWORD *) az_alloc(MAX_SEM_QUEUE_SIZE * sizeof(DWORD));
  s->value = v;
  s->rpos = s->wpos = 0;
  for(i=0; i<MAX_SEM_QUEUE_SIZE; i++)
    s->queue[i] = MAX_THREAD_ID;
  spinlock_irqsave_init(&s->lock);

  return 0;
}

/**
 * Destroy semaphore
 * return  0 on success
 *         EINVAL on invalid argument
 */
static inline int az_sem_destroy(sem_t* s)
{
  if (likelyval(!s, 0))
    return -EINVAL;

  spinlock_irqsave_destroy(&s->lock);
  az_free(s->queue);

  return 0;
}

/**
 * Nonblocking trywait for sempahore - Will return immediately if not available
 * return 0 on success (You got the semaphore)
 *        EINVAL on invalid argument
 *        ECANCELED on failure (You still have to wait)
 */
static inline int az_sem_trywait(sem_t* s)
{
  int ret = -ECANCELED;

  if (likelyval(!s, 0))
  return -EINVAL;

  spinlock_irqsave_lock(&s->lock);
  if (s->value > 0) {
    s->value--;
    mfence();
    ret = 0;
  }
  spinlock_irqsave_unlock(&s->lock);

  return ret;
}

/**
 * Blocking wait for semaphore
 * parameters: s Address of the according sem_t structure
 * parameters: ms Timeout in milliseconds
 * return 0 on success
 *        EINVAL on invalid argument
 *        ETIME on timer expired
 */
static inline int az_sem_wait(sem_t* s, DWORD ms)
{
  TCB *curr = get_current();

  if (likelyval(!s, 0))
  return -EINVAL;

  if (!ms) {
next_try1:
    spinlock_irqsave_lock(&s->lock);
    if (s->value > 0) {
      s->value--;
      mfence();
      spinlock_irqsave_unlock(&s->lock);
    } else {
      s->queue[s->wpos] = curr->id;
      s->wpos = (s->wpos + 1) % MAX_SEM_QUEUE_SIZE;
      spinlock_irqsave_unlock(&s->lock);
      schedule(THREAD_INTENTION_BLOCKED);
      goto next_try1;
    }
  } else {
#if 0
    DWORD ticks = (ms * TIMER_FREQ) / 1000;
    DWORD remain = (ms * TIMER_FREQ) % 1000;

    if (ticks) {
      QWORD deadline = get_ticks() + ticks;

next_try2:
      spinlock_irqsave_lock(&s->lock);
      if (s->value > 0) {
        s->value--;
        spinlock_irqsave_unlock(&s->lock);
        return 0;
      } else {
        if (get_ticks() >= deadline) {
          spinlock_irqsave_unlock(&s->lock);
          goto timeout;
        }
        s->queue[s->wpos] = curr->id;
        s->wpos = (s->wpos + 1) % MAX_NUM_THREAD;
        set_timer(deadline);
        spinlock_irqsave_unlock(&s->lock);
        reschedule();
        goto next_try2;
      }
    }

timeout:
    while (remain) {
      udelay(1000);
      remain--;

      if (!sem_trywait(s))
        return 0;
    }

    return -ETIME;
#endif
  }

  return 0;
}

/**
 * Give back resource
 * return 0 on success
 *        EINVAL on invalid argument
 */
static inline int az_sem_post(sem_t* s)
{
  if (likelyval(!s, 0))
    return -EINVAL;

  spinlock_irqsave_lock(&s->lock);

  s->value++;
  mfence();
  if (s->queue[s->rpos] < MAX_THREAD_ID) {
    thread_wake_up(s->queue[s->rpos]);
    s->queue[s->rpos] = MAX_THREAD_ID;
    s->rpos = (s->rpos + 1) % MAX_SEM_QUEUE_SIZE;
  }

  spinlock_irqsave_unlock(&s->lock);

  return 0;
}

// Functions
int sys_sem_init(sem_t** sem, unsigned int value);
int sys_sem_destroy(sem_t* sem);
int sys_sem_wait(sem_t* sem);
int sys_sem_post(sem_t* sem);
int sys_sem_timedwait(sem_t *sem, unsigned int ms);
int sys_sem_cancelablewait(sem_t* sem, unsigned int ms);

#endif  /* __SEMAPORT_H__ */
