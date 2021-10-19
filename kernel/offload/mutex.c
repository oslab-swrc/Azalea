// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "memory.h"
#include "mutex.h"

/****************************************************************************
 *
 * Mutexes
 *
****************************************************************************/

ticket_mutex_t *mutex_create(ticket_mutex_t *lock)
{
  lock = (ticket_mutex_t *) az_alloc((QWORD) sizeof(ticket_mutex_t));
  if(lock == NULL)
    return (NULL);

  lock->u.s.request = 0;
  lock->u.s.grant   = 0;
  
  return (lock);
}

int mutex_delete(ticket_mutex_t *lock)
{
  if(lock != NULL)
    az_free((void *) lock);

  lock = NULL;

  return (0);
}

int mutex_init(ticket_mutex_t *lock)
{
  if(lock == NULL)
    return (-1);

  lock->u.s.request = 0;
  lock->u.s.grant   = 0;
  
  return (0);
}

int mutex_lock(ticket_mutex_t *lock)
{

  int t = __sync_fetch_and_add(&lock->u.s.request, 1);

  while (lock->u.s.grant != t) {
    ;//__asm volatile ("pause" ::: "memory");
  }

  return (0);
}

int mutex_pause_lock(ticket_mutex_t *lock)
{

  int t = __sync_fetch_and_add(&lock->u.s.request, 1);

  while (lock->u.s.grant != t) {
    __asm volatile ("pause" ::: "memory");
  }

  return (0);
}

int mutex_unlock(ticket_mutex_t *lock)
{
  // COMPILER_BARRIER
  asm volatile ("" ::: "memory");

  lock->u.s.grant++;

  return (0);
}
