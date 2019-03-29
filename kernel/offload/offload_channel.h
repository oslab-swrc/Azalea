#ifndef __OFFLOAD_CHANNEL_H__
#define __OFFLOAD_CHANNEL_H__

#include <sys/lock.h>
#include "sync.h"

#define CQ_ELE_PAGE_NUM (130)           //
#define CQ_ELE_SIZE (130 * 4096)        // 8K + 512K

// Circular Queue
typedef struct cq_element_struct {
  char d[CQ_ELE_SIZE];
} cq_element;

struct circular_queue {
  int head;
  int tail;
  unsigned long size;
  spinlock_t *lock;
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
	int test;
  struct circular_queue *in;
  struct circular_queue *out;
  volatile int init_flag;
  //spinlock_t lock;
} channel_t;

BOOL init_offload_channel();
channel_t *get_offload_channel(int n_requested_channel);

#endif
