#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "offload_channel.h"
#include "offload_fio.h"
#include "offload_message.h"
#include "offload_mmap.h"


/* 
 * open
 */
void sys_off_open(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq)
{
  int  iret = -1;

  char *path = NULL;;
  int  oflag = 0;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  oflag = (int) in_pkt->param2;
  mode = (mode_t) in_pkt->param3;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute open
  if(mode == 0) {
    iret = open(path, oflag);
  }
  else {
    iret = open(path, oflag, mode);
  }

  if(iret == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_sys_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/* 
 * creat
 */
void sys_off_creat(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq)
{
  int  iret = -1;

  char *path = NULL;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  mode = (mode_t) in_pkt->param2;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute create
  iret = creat(path, mode);

  if(iret == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_sys_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/*
 * do_sys_off_read_v
 */
ssize_t do_sys_off_read_v(int fd, unsigned long iov_pa, int iovcnt)
{
	ssize_t readbytes = 0;
	ssize_t count = 0;

	struct iovec *iov = NULL;
	unsigned long iov_base_va = 0;

	int i = 0;

	iov = (struct iovec *) get_va(iov_pa);
	
	for(i = 0; i < iovcnt; i++) {
	  iov_base_va = get_va((unsigned long) iov->iov_base);
	  count = read(fd, (void *) iov_base_va, (size_t) iov->iov_len);

	  if(count == -1) {
		return -1;
	  } 

	  if(count < iov->iov_len) {
	    readbytes += count;
	    return readbytes;
	  }
	  
	  readbytes += count;
	}

	//fprintf(stdout, "\nread : fd=%d read bytes=%d", fd, readbytes);
	return readbytes;
}

/*
 * read 
 */
void sys_off_read(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq)
{

  ssize_t sret = 0;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  pa = (unsigned long) in_pkt->param2;
  count = (int) in_pkt->param3;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute read
  sret = do_sys_off_read_v(fd, pa, count);

  // check error
  if(sret  == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_sys_message(out_cq, tid, offload_function_type, (unsigned long) sret);
}

/*
 * do_sys_off_write_v
 */
ssize_t do_sys_off_write_v(int fd, unsigned long iov_pa, int iovcnt)
{
	ssize_t writebytes = 0;
	ssize_t count = 0;

	struct iovec *iov = NULL;
	unsigned long iov_base_va = 0;

	int i = 0;

	iov = (struct iovec *) get_va(iov_pa);
	
	for(i = 0; i < iovcnt; i++) {
	  iov_base_va = get_va((unsigned long) iov->iov_base);
	  count = write(fd, (void *) iov_base_va, (size_t) iov->iov_len);

	  if(count == -1) {
		return -1;
	  } 

	  if(count < iov->iov_len) {
	    writebytes += count;
	    return writebytes;
	  }
	  
	  writebytes += count;
	}

	//fprintf(stdout, "\nwrite : fd=%d write bytes=%d", fd, writebytes);
	return writebytes;
}

/*
 * write 
 */
void sys_off_write(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq)
{

  ssize_t sret = -1;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  pa = (unsigned long) in_pkt->param2;
  count = (int) in_pkt->param3;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute write
  sret = do_sys_off_write_v(fd, pa, count);

  // check error
  if(sret == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  send_sys_message(out_cq, tid, offload_function_type, (unsigned long) sret);
}

/*
 * close
 */
void sys_off_close(io_packet_t *in_pkt, struct circular_queue *in_cq, struct circular_queue *out_cq)
{
  int  iret = -1;

  int  fd = 0;

  int tid = 0;
  int  offload_function_type = 0;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute close
  iret = close(fd);

  // check error
  if(iret == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_sys_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

