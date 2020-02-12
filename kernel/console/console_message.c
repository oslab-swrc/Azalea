#include "console.h"
#include "console_message.h"

#define CONSOLE_LOCK_ENABLE

/**
 * @brief send offload console message
 * @param ocq circular queue
 * @param tid thread id
 * @param console_function_type console print call type
 * @param param1 1st parameter
 * @param param2 2nd parameter
 * @param param3 3rd parameter
 * @param param4 4th parameter
 * @param param5 5th parameter
 * @param param6 6th parameter
 * return none
 */
void send_console_message(struct circular_queue *ocq, int tid, int console_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6) 
{
cq_element *ce = NULL;
io_packet_t *opkt = NULL;

#ifdef CONSOLE_LOCK_ENABLE
  mutex_pause_lock(&ocq->lock);
  while (cq_free_space(ocq) == 0);
#else
  while (cq_free_space(ocq) == 0);
#endif

// make packet header
  ce = (cq_element *) (ocq->data + ocq->head);
  opkt = (io_packet_t *) ce;

  opkt->magic = MAGIC;
  opkt->tid = tid;
  opkt->io_function_type = console_function_type;
  opkt->param1 = param1;
  opkt->param2 = param2;
  opkt->param3 = param3;
  opkt->param4 = param4;
  opkt->param5 = param5;
  opkt->param6 = param6;

  ocq->head = (ocq->head + 1) % ocq->size;

#ifdef CONSOLE_LOCK_ENABLE
  mutex_unlock(&ocq->lock);
#endif
}

/**
 *@brief receive offload console message
 *@param icq circular queue
 *@param tid thread id
 *@param console_function_type console print call type
 *@return ret result of console print
 */
QWORD receive_console_message(struct circular_queue *icq, int tid, int console_function_type)
{
cq_element *ce = NULL;
io_packet_t *ipkt = NULL;
QWORD ret = 0;

retry_receive_sys_message:

#ifdef CONSOLE_LOCK_ENABLE
  mutex_pause_lock(&icq->lock);
  while (cq_avail_data(icq) == 0);
#else
  while (cq_avail_data(icq) == 0);
#endif
  ce = (cq_element *) (icq->data + icq->tail);
  ipkt= (io_packet_t *)(ce);
  if((int) ipkt->tid == (int) tid && (int) ipkt->io_function_type == (int) console_function_type) {
    ret = ipkt->ret;
    icq->tail = (icq->tail + 1) % icq->size;

#ifdef CONSOLE_LOCK_ENABLE
    mutex_unlock(&icq->lock);
#endif
  }
  else {
#ifdef CONSOLE_LOCK_ENABLE
    mutex_unlock(&icq->lock);
#endif
    lk_print("console: retry_receive_sys_message id: %d, thread id: %d\n", ipkt->tid, tid);
    goto retry_receive_sys_message;
  }

  return ret;
}
