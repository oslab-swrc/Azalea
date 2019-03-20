#include "offload_channel.h"
#include "offload_message.h"

/* 
 * send_sys_message
 */
void send_sys_message(struct circular_queue *out_cq, int tid, int offload_function_type, unsigned long  ret) 
{
cq_element *ce = NULL;
io_packet_t *out_pkt = NULL;

	// make packet header
	ce = (out_cq->data + out_cq->head);
	out_pkt = (io_packet_t *) ce;

	out_pkt->magic = MAGIC;
	out_pkt->tid = tid;
	out_pkt->io_function_type = offload_function_type;
	out_pkt->ret = ret;

	out_cq->head = (out_cq->head + 1) % out_cq->size;
}

