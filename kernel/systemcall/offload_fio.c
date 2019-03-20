#include <sys/lock.h>
#include <sys/uio.h>

//#include "map.h"
#include "console.h"
#include "az_types.h"
//#include "memory.h"
#include "offload_channel.h"
#include "offload_message.h"
#include "offload_fio.h"
#include "page.h"
#include "systemcalllist.h"
#include "utility.h"
#include "thread.h"

extern QWORD g_memory_start;

/*
 * get_iovec for buf(with count)
 * iov->iov_base has physical address
 */
int get_iovec(void *buf, size_t count, struct iovec *iov, int *iovcnt)
{
QWORD touch_buf = 0;
__attribute__((unused)) BYTE touch = 0;

QWORD page_addr = 0;		// physical address
QWORD prev_page_addr = 0;	// physical address
QWORD read_page_addr = 0;	// physical address
unsigned long read_page_len  = 0;
int offset_in_page = 0;

QWORD scan_buf = 0;			// virtual address
unsigned long scan_buf_len = 0;
unsigned long page_len = 0;

int i = 0;

  if(buf == NULL)
    return -1;
  
  // get page count of buf
  offset_in_page = ((QWORD) buf & PAGE_OFFSET_MASK);
  int nr_pages = (count + offset_in_page) / PAGE_SIZE_4K;
  (((count + offset_in_page) % PAGE_SIZE_4K) == 0) ? nr_pages : nr_pages++;

  // check page exists 
  touch_buf = (QWORD) buf - offset_in_page;
  for(i = 0; i < nr_pages; i++) {
	touch = *((BYTE *)(touch_buf + i * PAGE_SIZE_4K));
  }

  //set scan buf
  scan_buf = (QWORD) buf;
  scan_buf_len = count;

  // set initial page address
  page_addr = pa(buf) - offset_in_page;
  prev_page_addr = page_addr - PAGE_SIZE_4K;
  read_page_addr = page_addr + offset_in_page;
  read_page_len = 0;

  page_len = 0;

  /* scan each page to find physically contiguous regions */
  for (i = 0; i < nr_pages; ++i) {
	/* get physcical addresses */
	offset_in_page = scan_buf & PAGE_OFFSET_MASK;
	page_addr = pa(scan_buf) - offset_in_page;

	/* emmit iov for a physically non-contiguous region */
	if ((unsigned long) page_addr != (unsigned long) (prev_page_addr + PAGE_SIZE_4K)) {

	  iov[*iovcnt].iov_base = (void *) read_page_addr;
	  iov[*iovcnt].iov_len = (size_t) read_page_len;
	  (*iovcnt)++;

	  /* reset region info. */
	  read_page_len = 0;
	  read_page_addr = pa(scan_buf);
	}

	/* expand this region */
	page_len = PAGE_SIZE_4K - offset_in_page; //rest of page;
	if (page_len > scan_buf_len)
	  page_len = scan_buf_len;

	read_page_len += page_len;
	scan_buf += page_len;
	scan_buf_len -= page_len;
	prev_page_addr = page_addr;
  }

  iov[*iovcnt].iov_base = (void *) read_page_addr;
  iov[*iovcnt].iov_len = (size_t) read_page_len;
  (*iovcnt)++;

  return 0;
}

/* 
 * open
 */
int sys_off_open(const char *path, int oflag, mode_t mode)
{
int fd = 0;

channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
int mytid = -1;

  // chech parameter
  if(path == NULL) {
	return (-1);
  }

  ch = get_offload_channel(-1);
  //ch = get_offload_channel(1);
  if(ch == NULL) {
	  return (-1);
  }
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  send_sys_message(ocq, mytid, SYSCALL_sys_open, pa(path), oflag, mode, 0, 0, 0);
  fd = (int) receive_sys_message(icq, mytid, SYSCALL_sys_open);

  return fd;
}


/*
 * creat
 */
int sys_off_creat(const char *path, mode_t mode)
{
int fd = 0;

channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

  // chech parameter
  if(path == NULL) {
	return (-1);
  }

  ch = get_offload_channel(-1);
  if(ch == NULL) {
	  return -1;
  }
  icq = ch->in;
  ocq = ch->out;

  current = get_current(); 
  mytid = current->id;

  send_sys_message(ocq, mytid, SYSCALL_sys_creat, pa(path), mode, 0, 0, 0, 0);
  fd = (int) receive_sys_message(icq, mytid, SYSCALL_sys_creat);

  return fd;
}


/*
 * read
 */
ssize_t sys_off_read(int fd, void *buf, size_t count)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

ssize_t ret_count = 0;

struct iovec iov[MAX_IOV_NUM];
int iovcnt = 0;

  if(count <= 0) return (-1);

  ch = get_offload_channel(-1);
  if(ch == NULL) return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current(); 
  mytid = current->id;
  
  get_iovec(buf, count, iov, &iovcnt);
  send_sys_message(ocq, mytid, SYSCALL_sys_read, fd, pa(iov), iovcnt, 0, 0, 0); 
  ret_count = receive_sys_message(icq, mytid, SYSCALL_sys_read);

  return (ret_count);
}


/*
 * write
 */
ssize_t sys_off_write(int fd, void *buf, size_t count)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

ssize_t ret_count = 0;

struct iovec iov[MAX_IOV_NUM];
int iovcnt = 0;

  if(count <= 0) return (-1);

  ch = get_offload_channel(-1);
  if(ch == NULL) return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  get_iovec(buf, count, iov, &iovcnt);
  send_sys_message(ocq, mytid, SYSCALL_sys_write, fd, pa(iov), iovcnt, 0, 0, 0);
  ret_count = receive_sys_message(icq, mytid, SYSCALL_sys_write);

  return (ret_count);
}

/*
 * close
 */
int sys_off_close(int fd)
{
int iret = 0;

channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

  ch = get_offload_channel(-1);
  if(ch == NULL) {
          return -1;
  }

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  send_sys_message(ocq, mytid, SYSCALL_sys_close, fd, 0, 0, 0, 0, 0);
  iret = (int) receive_sys_message(icq, mytid, SYSCALL_sys_close);

  return iret;
}

