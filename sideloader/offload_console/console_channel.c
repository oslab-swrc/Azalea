// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdio.h>

#include "console_channel.h"

/**
 * @brief initialze circular queue
 * @param cq circular queue
 * @param size circular queue size
 * @return none
 */
void cq_init(struct circular_queue *cq, unsigned long size)
{
  cq->head = 0;
  cq->tail = 0;
  cq->size = size;
}

/**
 * @brief cq_avail_data()
 * @param cq cicular queue
 * @return (availalbe data size)
 */
int cq_avail_data(struct circular_queue *cq)
{
  return (cq->head - cq->tail) & (cq->size - 1);
}

/*
 * @brief cq_free_space()
 * @param cq circular queue
 * @return (free space size)
 */
int cq_free_space(struct circular_queue *cq)
{
  return (cq->size - 1 - cq_avail_data(cq));
}

/**
 * @brief cq_add_data()
 * Call this function after confirming enough room is available
 * @param cq circular queue
 * @param data to add
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
 * @brief cq_remove_data()
 * Call this function after confirming requested size of data is available
 * @param cq circular queue
 * @param data to add
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
  memset(ioc, 0, sizeof(channel_t));
}

