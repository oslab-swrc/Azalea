#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "console_channel.h"
#include "console_function.h"
#include "console_message.h"
#include "console_mmap.h"

/**
 * @brief execute write console call
 * do write according to struct iovec data
 * @param fd file descriptor
 * @param iov_pa iovec data
 * @param iovcnt iovec count
 * @return success (written bytes), fail (-1)
 */
ssize_t do_console_off_write_v(int fd, unsigned long iov_pa, int iovcnt)
{
  ssize_t writebytes = 0;
  ssize_t count = 0;

  struct iovec *iov = NULL;
  unsigned long iov_base_va = 0;

  int i = 0;

  iov = (struct iovec *) get_va(iov_pa);

  for(i = 0; i < iovcnt; i++) {
    iov_base_va = get_va((unsigned long) iov[i].iov_base);
    count = write(fd, (void *) iov_base_va, (size_t) iov[i].iov_len);

    if(count == -1) {
      return -1;
    }

    if(fd == 1)
      fflush(stdout);
    else if(fd == 2)
      fflush(stderr);

    if(count < iov[i].iov_len) {
      writebytes += count;
      return writebytes;
    }

    writebytes += count;
  }

  return writebytes;
}


/**
 * @brief execute write console call
 * write()  writes  up  to  count bytes from the buffer pointed buf
 * to the file referred to by the file descriptor fd(stderr, stderr).
 * @param channel
 * @return none
 */
void console_off_write(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  ssize_t sret = -1;

  int fd = 0;
  unsigned long iov_pa = 0;
  int iovcnt = 0;

  int tid = 0;
  int  console_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  console_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  iov_pa = (unsigned long) in_pkt->param2;
  iovcnt = (int) in_pkt->param3;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;
 
  // execute write console
  sret = do_console_off_write_v(fd, iov_pa, iovcnt);

  // check error
  if(sret == -1)
    fprintf(stderr, "CONSOLE WRITE: %s, %d\n", strerror(errno), fd);
  
  send_console_message(out_cq, tid, console_function_type, (unsigned long) sret);
}


/**
 * @brief execute print
 * do print according to struct iovec data 
 * @param iov_pa iovec data 
 * @param iovcnt iovec count 
 * @return 
 */
int do_print_v(unsigned long iov_pa, int iovcnt)
{
	struct iovec *iov = NULL;
	unsigned long iov_base_va = 0;

	int i = 0;

	iov = (struct iovec *) get_va(iov_pa);
        if(iov == NULL) 
	  return (-1);

	for(i = 0; i < iovcnt; i++) {
	  iov_base_va = get_va((unsigned long) iov[i].iov_base);
	  if(iov_base_va != 0) { 
	    printf("%s", (char *) iov_base_va);
	    fflush(stdout);
	  }
	}

	return (0);
}

/**
 * @brief execute print
 * console_print()  prints buf 
 * @param channel 
 * @return none
 */
void console_print(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int console_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  console_function_type = (int) in_pkt->io_function_type;

  pa = (unsigned long) in_pkt->param1;
  count = (int) in_pkt->param2;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute write
  iret = do_print_v(pa, count);

  // check error
  if(iret == -1)
	fprintf(stderr, "PRINT: %s\n", strerror(errno));

  send_console_message(out_cq, tid, console_function_type, (unsigned long) iret);
}


/**
 * @brief execute getch() 
 * console_getch()  get character  
 * @param channel 
 * @return none
 */
void console_getch(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int cret = '\0';;

  int tid = 0;
  int console_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  console_function_type = (int) in_pkt->io_function_type;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute write
  cret = getchar();

  // check error
  if(cret == ERR)
    fprintf(stderr, "GETCH: %s\n", strerror(errno));

  send_console_message(out_cq, tid, console_function_type, (unsigned long) cret);
}

