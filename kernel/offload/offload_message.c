#include "console.h"
#include "offload_message.h"

#define OFFLOAD_LOCK_ENABLE

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

#ifdef OFFLOAD_LOCK_ENABLE
	//spinlock_lock(ocq->lock);
	//spinlock_lock(&ocq->lock);
	//spinlock_irqsave_lock(&ocq->lock);
	mutex_lock(&ocq->lock);
	while (cq_free_space(ocq) == 0);
#else
	while (cq_free_space(ocq) == 0);
#endif
	// make packet header
	ce = (ocq->data + ocq->head);
	opkt = (io_packet_t *) ce;

	opkt->magic = MAGIC;
	opkt->tid = tid;
	opkt->io_function_type = offload_function_type;
	opkt->param1 = param1;
	opkt->param2 = param2;
	opkt->param3 = param3;
	opkt->param4 = param4;
	opkt->param5 = param5;
	opkt->param6 = param6;

	ocq->head = (ocq->head + 1) % ocq->size;

#ifdef OFFLOAD_LOCK_ENABLE
	  //spinlock_unlock(ocq->lock);
	//spinlock_unlock(&ocq->lock);
	//spinlock_irqsave_unlock(&ocq->lock);
	mutex_unlock(&ocq->lock);
#endif
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

retry_receive_sys_message:
#ifdef OFFLOAD_LOCK_ENABLE
	//spinlock_lock(icq->lock);
	//spinlock_lock(&icq->lock);
	//spinlock_irqsave_lock(&icq->lock);
	mutex_lock(&icq->lock);
	while (cq_avail_data(icq) == 0);
#else
	while (cq_avail_data(icq) == 0);
#endif

	ce = (icq->data + icq->tail);
	ipkt= (io_packet_t *)(ce);
	if((int) ipkt->tid == (int) tid && (int) ipkt->io_function_type == (int) offload_function_type) {
		ret = ipkt->ret;
		icq->tail = (icq->tail + 1) % icq->size;

#ifdef OFFLOAD_LOCK_ENABLE
		//spinlock_unlock(icq->lock);
		//spinlock_unlock(&icq->lock);
		//spinlock_irqsave_unlock(&icq->lock);
		mutex_unlock(&icq->lock);
#endif
	}
	else {
#ifdef OFFLOAD_LOCK_ENABLE
		//spinlock_unlock(icq->lock);
		//spinlock_unlock(&icq->lock);
		//`spinlock_irqsave_unlock(&icq->lock);
		mutex_unlock(&icq->lock);
		lk_print_xy(0, 4, " receive retry %d, %d : lock enabled", tid, offload_function_type);
#endif
		lk_print_xy(0, 5, " receive retry %d, %d", tid, offload_function_type);
		//schedule(THREAD_INTENTION_READY);
		goto retry_receive_sys_message;
	}

	return ret;
}

