#include "console.h"
#include "offload_message.h"
#include "thread.h"

extern int g_ukid;

/**
 * @brief send offload message
 * @param ocq circular queue
 * @param tid thread id
 * @param offload_function_type system call type
 * @param param1 1st parameter
 * @param param2 2nd parameter
 * @param param3 3rd parameter
 * @param param4 4th parameter
 * @param param5 5th parameter
 * @param param6 6th parameter
 * return none
 */
void send_offload_message(struct circular_queue *ocq, int tid, int offload_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6)
{
cq_element *ce = NULL;
io_packet_t *opkt = NULL;
int offload_tid = 0;
int check_count = 0;

send_retry:

  spinlock_lock(&ocq->slock);

check_ocq:

  if(cq_free_space(ocq) != 0) {

    offload_tid = g_ukid * 10000 + tid;

    ce = (cq_element *) (ocq->data + ocq->head);
    opkt = (io_packet_t *) ce;

    opkt->magic = OFFLOAD_MAGIC;
    opkt->tid = offload_tid;
    opkt->io_function_type = offload_function_type;
    opkt->param1 = param1;
    opkt->param2 = param2;
    opkt->param3 = param3;
    opkt->param4 = param4;
    opkt->param5 = param5;
    opkt->param6 = param6;

    ocq->head = (ocq->head + 1) % ocq->size;

    mfence();
    spinlock_unlock(&ocq->slock);
  }
  else {
    if(check_count < 1000) {
      check_count++;
      goto check_ocq;
    }

    mfence();
    spinlock_unlock(&ocq->slock);

    get_current()->remaining_time_slice = 0;
    schedule(THREAD_INTENTION_READY);
    goto send_retry;
  }
}


/**
 *@brief receive offload message
 *@param icq circular queue
 *@param tid thread id
 *@param offload_function_type system call type
 *@return ret result of system call
 */
QWORD receive_offload_message(struct circular_queue *icq, int tid, int offload_function_type)
{
cq_element *ce = NULL;
io_packet_t *ipkt = NULL;
QWORD ret = 0;
int offload_tid = 0;
int check_count = 0;

receive_retry:
    spinlock_lock(&icq->slock);

    check_count = 0;
check_icq:

  if (cq_avail_data(icq)) {
    ce = (icq->data + icq->tail);
    ipkt= (io_packet_t *)(ce);
    offload_tid = g_ukid * 10000 + tid;
    if((int) ipkt->tid == (int) offload_tid) {
      ret = ipkt->ret;
      icq->tail = (icq->tail + 1) % icq->size;

      mfence();
      spinlock_unlock(&icq->slock);
    }
    else {
      mfence();
      spinlock_unlock(&icq->slock);
      get_current()->remaining_time_slice = 0;
      schedule_to((int) (ipkt->tid - g_ukid * 10000), THREAD_INTENTION_READY);

      goto receive_retry;
    }
  } else {

    if(check_count < 1000) {
      check_count++;
      goto check_icq;
    }

    mfence();
    spinlock_unlock(&icq->slock);

    get_current()->remaining_time_slice = 0;
    schedule(THREAD_INTENTION_READY);

    goto receive_retry;
  }

  return ret;
}
