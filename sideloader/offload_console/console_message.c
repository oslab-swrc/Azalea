#include <stdio.h>

#include "console_channel.h"
#include "console_message.h"

/**
 * @brief send console message
 * @param out_cq circular queue
 * @param in_cq circular queue
 * @param console_function_type system call type
 * @param ret the result of console pirnt call
 * return none
 */
void send_console_message(struct circular_queue *out_cq, int tid, int console_function_type, unsigned long  ret) 
{
cq_element *ce = NULL;
io_packet_t *out_pkt = NULL;

	// make packet header
	ce = (out_cq->data + out_cq->head);
	out_pkt = (io_packet_t *) ce;

	out_pkt->magic = MAGIC;
	out_pkt->tid = (unsigned long) tid;
	out_pkt->io_function_type = (unsigned long) console_function_type;
	out_pkt->ret = ret;

	out_cq->head = (out_cq->head + 1) % out_cq->size;
}

