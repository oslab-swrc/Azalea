#ifndef __OFFLOAD_CHANNEL_H__
#define __OFFLOAD_CHANNEL_H__

#define	PAGE_SIZE_4K	(0x1000)
#define	PAGE_SIZE_2M	(0x200000)

#define CQ_ELE_PAGE_NUM (130)		// 2 + 128 
#define CQ_ELE_SIZE	(130 * 4096)	// 8K + 512K

#define   LOCK_ENABLED
#define   LOCK_ENABLED_MAX_CHANNELS_NUM        320

#ifdef LOCK_ENABLED
typedef struct az_spinlock_struct {
  volatile unsigned long lock;
} az_spinlock_t __attribute__ ((aligned (8)));

static inline void az_spinlock_init(az_spinlock_t *lock)
{
  lock->lock = 0;
}
#endif

// Circular queue struct
typedef struct cq_element_struct {
  char d[CQ_ELE_SIZE];
} cq_element;

struct circular_queue {
  int head;
  int tail;
  unsigned long size;
  cq_element data[0] __attribute__((aligned(4096)));
};

// Circular queue function
void cq_init(struct circular_queue *cq, unsigned long size);
int cq_avail_data(struct circular_queue *cq);
int cq_free_space(struct circular_queue *cq);
int cq_add_data(struct circular_queue *cq, char data);
int cq_remove_data(struct circular_queue *cq, char *data);

// Channel struct
typedef struct channel_struct {
  struct circular_queue *out_cq;
  unsigned long out_cq_len;

  int in_connected;
  struct circular_queue *in_cq;
  unsigned long in_cq_len;
} channel_t;

// Channel function
void init_channel(channel_t *ioc);

#endif
