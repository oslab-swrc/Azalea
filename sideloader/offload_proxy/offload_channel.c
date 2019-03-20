#include <string.h>
#include <stdio.h>

#include "offload_channel.h"

/*
 * cq_init()
 */
void cq_init(struct circular_queue *cq, unsigned long size)
{
  cq->head = 0;
  cq->tail = 0;
  cq->size = size;
}

/*
 * cq_avail_data()
 */
int cq_avail_data(struct circular_queue *cq)
{
  return (cq->head - cq->tail) & (cq->size - 1);
}

/*
 * cq_free_space()
 */
int cq_free_space(struct circular_queue *cq)
{
  return (cq->size - 1 - cq_avail_data(cq));
}

/*
 * cq_add_data()
 * Call this function after confirming enough room is available
 */
int cq_add_data(struct circular_queue *cq, char data)
{
  while (cq_free_space(cq) == 0);
  (cq->data + cq->head)->d[0] = data;
//    mbarrier();
  cq->head = (cq->head + 1) % cq->size;

  return 1;
}

/*
 * cq_remove_data()
 * Call this function after confirming requested size of data is available
 */
int cq_remove_data(struct circular_queue *cq, char *data)
{
  while (cq_avail_data(cq) == 0);
  *data = (cq->data + cq->tail)->d[0];
//    mbarrier();
  cq->tail = (cq->tail + 1) % cq->size;

  return 1;
}

/*
 * init_channel()
 */
void init_channel(channel_t *ioc)
{
  memset(ioc, 0, sizeof(channel_t));
}

