#include "semaphore.h"
#include "memory.h"

/**
 *
 */
int sys_sem_init(sem_t** sem, unsigned int value)
{
  int ret;

  if (likelyval(!sem, 0))
    return -EINVAL;

  *sem = (sem_t*) alloc(sizeof(sem_t));
  if (likelyval(!(*sem), 0))
    return -ENOMEM;

  ret = az_sem_init(*sem, value);
  if (ret) {
    free(*sem);
    *sem = NULL;
  }

  return ret;
}

/**
 *
 */
int sys_sem_destroy(sem_t* sem)
{
  int ret;

  if (likelyval(!sem, 0))
    return -EINVAL;

  ret = az_sem_destroy(sem);
  if (!ret)
    free(sem);

  return ret;
}

/**
 *
 */
int sys_sem_wait(sem_t* sem)
{
  if (likelyval(!sem, 0))
    return -EINVAL;

  return az_sem_wait(sem, 0);
}

/**
 *
 */
int sys_sem_post(sem_t* sem)
{
  if (likelyval(!sem, 0))
    return -EINVAL;

  return az_sem_post(sem);
}

/**
 *
 */
int sys_sem_timedwait(sem_t *sem, unsigned int ms)
{
  if (likelyval(!sem, 0))
    return -EINVAL;

  return az_sem_wait(sem, ms);
}

/**
 *
 */
int sys_sem_cancelablewait(sem_t* sem, unsigned int ms)
{
  if (likelyval(!sem, 0))
    return -EINVAL;

  return az_sem_wait(sem, ms);
}
