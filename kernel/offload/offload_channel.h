#ifndef __OFFLOAD_CHANNEL_H__
#define __OFFLOAD_CHANNEL_H__

#include "mutex.h"

#define CQ_ELE_PAGE_NUM (2)             // 2
#define CQ_ELE_SIZE     (2 * 4096)      // 8K

#define OFFLOAD_MAGIC	(0x5D4C3B2A)

#define L_CACHE_LINE_SIZE       64

// Circular Queue
typedef struct cq_element_struct {
  char d[CQ_ELE_SIZE];
} cq_element;

struct circular_queue {
  int head;
  int tail;
  unsigned long size;
  char padding[64] __attribute__((aligned(L_CACHE_LINE_SIZE))); 
  ticket_mutex_t lock __attribute__((aligned(L_CACHE_LINE_SIZE)));

  cq_element data[0] __attribute__((aligned(4096)));
};

void cq_init(struct circular_queue *cq, unsigned long size);
int cq_avail_data(struct circular_queue *cq);
int cq_free_space(struct circular_queue *cq);
int cq_add_data(struct circular_queue *cq, char data);
int cq_remove_data(struct circular_queue *cq, char *data);
int cq_is_full(struct circular_queue *cq);
int cq_is_empty(struct circular_queue *cq);

// Offload Channel
typedef struct channel_struct {
  struct circular_queue *in;
  struct circular_queue *out;
  volatile int init_flag;
} channel_t;

void init_channel(channel_t *ioc);

#endif
