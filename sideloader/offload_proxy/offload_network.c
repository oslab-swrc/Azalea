#include "offload_network.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "offload_channel.h"
#include "offload_message.h"
#include "offload_mmap.h"

/**
 * @brief Receive the request from the queue, Execute gethostname systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_gethostname(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int ret = 0;
  int offload_function_type = 0;
  char *name = 0;
  int len = 0;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  name = (char *) get_va(in_pkt->param1);
  len = (int) in_pkt->param2;

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute gethostname systemcall
  ret = gethostname(name, len); 
  if(ret == -1)
	fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (int) ret);
}

/**
 * @brief Receive the request from the queue, Execute gethostbyname systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_gethostbyname(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  char *name;
  struct hostent *ret;

  char *h_name;
  char *h_aliase;
  int *h_addrtype;
  int *h_length;
  char *h_addr;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  name = (char *) get_va(in_pkt->param1);

  h_name = (char *) get_va(in_pkt->param2);
  h_aliase = (char *) get_va(in_pkt->param3);
  h_addrtype = (int *) get_va(in_pkt->param4);
  h_length = (int *) get_va(in_pkt->param5);
  h_addr = (char *) get_va(in_pkt->param6);

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute gethostbyname systemcall
  ret = gethostbyname(name);

  if(ret != NULL) {
    if(ret->h_name != NULL) {
      //printf("h_name: %s\n", ret->h_name);
      strcpy(h_name, ret->h_name);
    }

    if(ret->h_aliases[0] != NULL) {
      strcpy(h_aliase, ret->h_aliases[0]);
    }

    *h_addrtype = ret->h_addrtype;
    *h_length = ret->h_length;

    if(ret->h_addr_list[0] != NULL && ret->h_length > 0) {
      //printf("h_addr: %s\n", ret->h_addr);
      memcpy(h_addr, ret->h_addr, ret->h_length);
    }
  }

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) ret);
}

/**
 * @brief Receive the request from the queue, Execute getsockname systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_getsockname(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int sockfd = 0;
  struct sockaddr *addr;
  socklen_t *addrlen;
  int iret = -1;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  sockfd = (int) in_pkt->param1;
  addr = (struct sockaddr *) get_va(in_pkt->param2);
  addrlen = (socklen_t *) get_va(in_pkt->param3);

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute getsockname systemcall
  iret = getsockname(sockfd, addr, addrlen);
  if(iret == -1)
    fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief Receive the request from the queue, Execute socket systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_socket(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int domain = 0;
  int type = 0;
  int protocol = 0;
  int iret = -1;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  domain = (int) in_pkt->param1;
  type = (int) in_pkt->param2; 
  protocol = (int) in_pkt->param3;

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute socket systemcall
  iret = socket(domain, type, protocol);
  if(iret == -1)
    fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief Receive the request from the queue, Execute bind systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_bind(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int sockfd = 0;
  struct sockaddr *addr;
  socklen_t addrlen;
  int iret = 0;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  sockfd = (int) in_pkt->param1;
  addr = (struct sockaddr *) get_va(in_pkt->param2);
  addrlen = (socklen_t) in_pkt->param3;

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute bind systemcall
  iret = bind(sockfd, addr, addrlen);
  if(iret == -1)
    fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief Receive the request from the queue, Execute listen systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_listen(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int sockfd = 0;
  int backlog = 0;
  int iret = -1;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  sockfd = (int) in_pkt->param1;
  backlog = (int) in_pkt->param2;

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute listen systemcall
  iret = listen(sockfd, backlog);
  if(iret == -1)
    fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief Receive the request from the queue, Execute connect systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_connect(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int sockfd = 0;
  struct sockaddr *addr;
  socklen_t addrlen;
  int iret = -1;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  sockfd = (int) in_pkt->param1;
  addr = (struct sockaddr *) get_va(in_pkt->param2);
  addrlen = (socklen_t) in_pkt->param3; 

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute connect systemcall
  iret = connect(sockfd, addr, addrlen);
  if(iret == -1)
    fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief Receive the request from the queue, Execute accept systemcall then return the result
 * @param ch- channel defined and allocated in the shared memory
 * @return none
 */
void sys_off_accept(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;
  int tid = 0;
  int offload_function_type = 0;
  int sockfd = 0;
  struct sockaddr *addr;
  socklen_t *addrlen;
  int iret = -1;

  // Get the queue information
  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  // Receive parameters passed from the queue
  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;
  sockfd = (int) in_pkt->param1;
  addr = (struct sockaddr *) get_va(in_pkt->param2);
  addrlen = (socklen_t *) get_va(in_pkt->param3);

  // Empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // Execute accept systemcall
  iret = accept(sockfd, addr, addrlen);
  if(iret == -1)
      fprintf(stderr, "%s\n", strerror(errno));

  // Sent to the result to the kernel
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}
