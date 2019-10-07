#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <stdint.h>

/****************************************************************************
 *
 * Mutexes
 *
****************************************************************************/
#define L_CACHE_LINE_SIZE       64
#define LOCKED			0
#define UNLOCKED		1

typedef union __ticket_lock {
    volatile uint64_t u;
    struct {
        volatile uint32_t grant;
        volatile uint32_t request;
    } s;
} ticket_lock_t __attribute__((aligned(L_CACHE_LINE_SIZE)));

typedef struct ticket_mutex {
    ticket_lock_t u __attribute__((aligned(L_CACHE_LINE_SIZE)));
} ticket_mutex_t __attribute__((aligned(L_CACHE_LINE_SIZE)));


ticket_mutex_t *mutex_create(ticket_mutex_t *lock);
int mutex_delete(ticket_mutex_t *lock);
int mutex_init(ticket_mutex_t *lock);
int mutex_lock(ticket_mutex_t *lock);
int mutex_unlock(ticket_mutex_t *lock);

#endif  /*__MUTEX_H__*/
