#include "console.h"
#include "offload_message.h"

//#define   LOCK_ENABLED



void send_sys_message(struct circular_queue *ocq, int tid, int offload_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6) 
{
cq_element *ce = NULL;
io_packet_t *opkt = NULL;

#ifdef LOCK_ENABLED
	spinlock_lock(ocq->lock);
	while (cq_free_space(ocq) == 0);
#else
	while (cq_free_space(ocq) == 0);
#endif
        //lk_print_xy(0, 1, "send msg : pa1:%Q, pa2:%Q, pa3:%Q", param1, param2, param3);
        //lk_print_xy(0, 2, "send msg : pa1:%d, pa2:%d, pa3:%d", param1, param2, param3);
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

        //lk_print_xy(0, 3, "send msg : ocq->head:%d, ocq->tail:%d, size:%d", ocq->head, ocq->tail, ocq->size);
	ocq->head = (ocq->head + 1) % ocq->size;

#ifdef LOCK_ENABLED
	  spinlock_unlock(ocq->lock);
#endif
}


QWORD receive_sys_message(struct circular_queue *icq, int tid, int offload_function_type)
{
cq_element *ce = NULL;
io_packet_t *ipkt = NULL;
QWORD ret = 0;

retry_receive_sys_message:
#ifdef LOCK_ENABLED
	spinlock_lock(icq->lock);
	while (cq_avail_data(icq) == 0);
#else
	while (cq_avail_data(icq) == 0);
#endif

	ce = (icq->data + icq->tail);
	ipkt= (io_packet_t *)(ce);
	if((int) ipkt->tid == (int) tid && (int) ipkt->io_function_type == (int) offload_function_type) {
		ret = ipkt->ret;
		icq->tail = (icq->tail + 1) % icq->size;

#ifdef LOCK_ENABLED
	spinlock_unlock(icq->lock);
#endif
	}
	else {
#ifdef LOCK_ENABLED
		spinlock_unlock(icq->lock);
#endif
		lk_print_xy(0, 5, " receive retry %d, %d", tid, offload_function_type);
		goto retry_receive_sys_message;
	}

	return ret;
}

