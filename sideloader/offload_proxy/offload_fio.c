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

/**
 * @brief execute open system call
 * @param channel 
 * @return none
 */
void sys_off_open(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;
  int  oflag = 0;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  oflag = (int) in_pkt->param2;
  mode = (mode_t) in_pkt->param3;

#ifdef DEBUG
  printf("\nopen system call: path=%s, flags=%d, mode=%d", path, oflag, mode);
#endif

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
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute creat system call
 * @param channel 
 * @return none
 */
void sys_off_creat(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  mode = (mode_t) in_pkt->param2;

#ifdef DEBUG
  printf("\ncreat system call: path=%s, mode=%d", path, mode);
#endif

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute create
  iret = creat(path, mode);

  if(iret == -1)
	fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute read system call
 * do read according to struct iovec data 
 * @param fd file descriptor
 * @param iov_pa iovec data
 * @param iovcntr iovec count 
 * @return success (read bytes), fail (-1)
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

	return readbytes;
}

/**
 * @brief execute write system call
 * @param channel 
 * @return none
 */
void sys_off_read(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  ssize_t sret = 0;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

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
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) sret);
}

/**
 * @brief execute write system call
 * do write according to struct iovec data 
 * @param fd file descriptor 
 * @param iov_pa iovec data 
 * @param iovcnt iovec count 
 * @return success (written bytes), fail (-1)
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

/**
 * @brief execute write system call
 * write()  writes  up  to  count bytes from the buffer pointed buf 
 * to the file referred to by the file descriptor fd.
 * @param channel 
 * @return none
 */
void sys_off_write(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  ssize_t sret = -1;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

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

  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) sret);
}

/**
 * @brief execute close system call
 * close()  closes  a  file descriptor.
 * @param channel 
 * @return none
 */
void sys_off_close(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  int  fd = 0;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

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
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}


/**
 * @brief execute lseek system call
 * lseek() function repositions the offset of the open file.
 * @param channel 
 * @return none
 */
void sys_off_lseek(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  off_t  oret = -1;

  int  fd = 0;
  off_t offset = 0;
  int whence = 0;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  offset = (off_t) in_pkt->param2;
  whence = (int) in_pkt->param3;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute lseek
  oret = lseek(fd, offset, whence);

  // check error
  if(oret == -1)
        fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) oret);
}


/**
 * @brief execute unlink system call
 * unlink()  deletes a name from the file system.
 * @param channel 
 * @return none
 */
void sys_off_unlink(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute unlink
  iret = unlink(path);

  // check error
  if(iret == -1)
    fprintf(stdout, "%s\n", strerror(errno));

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

