#include "offload_channel.h"
#include "utility.h"

/**
 * @brief initialize circular queue
 * @param cq pointer to circular queue
 * @param size the size of circular queue
 * @return none
 */
void cq_init(struct circular_queue *cq, unsigned long size)
{
  cq->head = 0;
  cq->tail = 0;
  cq->size = size;
}

/**
 * @brief return the aviilable size of circular queue
 * @param cq pointer to circular queue
 * @return available data size
 */
int cq_avail_data(struct circular_queue *cq)
{
  return (cq->head - cq->tail) & (cq->size - 1);
}

/**
 * @brief return the free space size of circular queue
 * @param cq pointer to circular queue
 * @return free space size
 */
int cq_free_space(struct circular_queue *cq)
{
  return (cq->size - 1 - cq_avail_data(cq));
}

/**
 * @brief return circular queue is full or not
 * @param cq pointer to circular queue
 * @return  if full 1 else 0
 */
int cq_is_full(struct circular_queue *cq)
{
  return ((cq->tail + 1) % cq->size == cq->head);
}

/**
 * @brief return circular queue is empty or not
 * @param cq pointer to circular queue
 * @return  if empty 1 else 0
 */
int cq_is_empty(struct circular_queue *cq)
{
  return (cq->head == cq->tail);
}

/**
 * @brief add data to circular cueue
 * Call this function after confirming enough room is available
 * @param cq pointer to circular queue
 * @param data data to add
 * @return (1)
 */
int cq_add_data(struct circular_queue *cq, char data)
{
  while (cq_free_space(cq) == 0);
  (cq->data + cq->head)->d[0] = data;
//    mbarrier();
  cq->head = (cq->head + 1) % cq->size;

  return 1;
}

/**
 * @brief remove data from circular queue
 * Call this function after confirming requested size of data is available
 * @param cq pointer to circular queue
 * @param data data to remove
 * @return (1)
 */
int cq_remove_data(struct circular_queue *cq, char *data)
{
  while (cq_avail_data(cq) == 0);
  *data = (cq->data + cq->tail)->d[0];
//    mbarrier();
  cq->tail = (cq->tail + 1) % cq->size;

  return 1;
}

/**
 * @brief initialize channel
 * @param ioc channel
 * @return none
 */
void init_channel(channel_t *ioc)
{
  lk_memset(ioc, 0, sizeof(channel_t));
}
