#ifndef __OFFLOAD_IO_H__
#define __OFFLOAD_IO_H__

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_open(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq);
void sys_off_creat(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq);
void sys_off_read(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq);
void sys_off_write(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq);
void sys_off_close(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq);

#endif
