#include <sys/lock.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "console.h"
#include "az_types.h"
#include "offload_channel.h"
#include "offload_mmap.h"
#include "offload_message.h"
#include "offload_fio.h"
#include "page.h"
#include "memory.h"
#include "systemcalllist.h"
#include "utility.h"
#include "thread.h"

extern QWORD g_memory_start;

/**
 * @brief get_iovec for buf(with count)
 * iov->iov_base contain physical address
 * @param buf pointer to buf
 * @param count buf size
 * @param iov pointer to struct iovec which contains iov_base and iov_len
 * @param iovcnt count of iov
 * return success (0), fail (-1)
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
  page_addr = get_pa((QWORD) buf) - offset_in_page;
  prev_page_addr = page_addr - PAGE_SIZE_4K;
  read_page_addr = page_addr + offset_in_page;
  read_page_len = 0;

  page_len = 0;

  /* scan each page to find physically contiguous regions */
  for (i = 0; i < nr_pages; ++i) {
	/* get physcical addresses */
	offset_in_page = scan_buf & PAGE_OFFSET_MASK;
	page_addr = get_pa(scan_buf) - offset_in_page;

	/* emmit iov for a physically non-contiguous region */
	if ((unsigned long) page_addr != (unsigned long) (prev_page_addr + PAGE_SIZE_4K)) {

	  iov[*iovcnt].iov_base = (void *) read_page_addr;
	  iov[*iovcnt].iov_len = (size_t) read_page_len;
	  (*iovcnt)++;

	  /* reset region info. */
	  read_page_len = 0;
	  read_page_addr = get_pa(scan_buf);
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

/** 
 * @brief file open system call
 * @param path for a file
 * @param oflag open flag
 * @param mode open mode
 * @return success (file descriptor), fail (-1)
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

  send_offload_message(ocq, mytid, SYSCALL_sys_open, get_pa((QWORD) path), oflag, mode, 0, 0, 0);
  fd = (int) receive_offload_message(icq, mytid, SYSCALL_sys_open);

  return fd;
}


/**
 * @brief file create system call
 * @param path for a file
 * @param mode create mode
 * @return success (file descriptor), fail (-1)
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

  send_offload_message(ocq, mytid, SYSCALL_sys_creat, get_pa((QWORD) path), mode, 0, 0, 0, 0);
  fd = (int) receive_offload_message(icq, mytid, SYSCALL_sys_creat);

  return fd;
}


/**
 * @brief read system call
 * @param fd file descriptor
 * @param buf buffer
 * @param count count bytes to read
 * @return success (the number of bytes read), fail (-1)
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
  send_offload_message(ocq, mytid, SYSCALL_sys_read, fd, get_pa((QWORD) iov), iovcnt, 0, 0, 0); 
  ret_count = receive_offload_message(icq, mytid, SYSCALL_sys_read);

  return (ret_count);
}


/**
 * @brief write system call
 * @param fd file descriptor
 * @param buf buffer
 * @param count count bytes to write
 * @return success (the number of bytes written), fail (-1)
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
  send_offload_message(ocq, mytid, SYSCALL_sys_write, fd, get_pa((QWORD) iov), iovcnt, 0, 0, 0);
  ret_count = receive_offload_message(icq, mytid, SYSCALL_sys_write);

  return (ret_count);
}

/**
 * @brief file close system call
 * @param fd file descriptor
 * @return success (0), fail (-1)
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

  send_offload_message(ocq, mytid, SYSCALL_sys_close, fd, 0, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_close);

  return iret;
}


/**
 * @brief file lseek system call
 * @param fd file descriptor
 * @param offset offset bytes to reposition
 * @param whence the directive
 * @return success (0), fail (-1)
 */
off_t sys_off_lseek(int fd, off_t offset, int whence)
{
off_t oret = 0;

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

  send_offload_message(ocq, mytid, SYSCALL_sys_lseek, fd, offset, whence, 0, 0, 0);
  oret = (off_t) receive_offload_message(icq, mytid, SYSCALL_sys_lseek);

  return oret;
}

/**
 * @brief file link system call
 * @param oldpath for an existing file
 * @param newpath for a new link 
 * @return success (0), fail (-1)
 */
int sys_off_link(const char *oldpath, const char *newpath)
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

  send_offload_message(ocq, mytid, SYSCALL_sys_link, get_pa((QWORD) oldpath), get_pa((QWORD) newpath), 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_link);

  return iret;
}

/**
 * @brief file unlink system call
 * @param path for a file
 * @return success (0), fail (-1)
 */
int sys_off_unlink(const char *path)
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

  send_offload_message(ocq, mytid, SYSCALL_sys_unlink, get_pa((QWORD) path), 0, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_unlink);

  return iret;
}

/**
 * @brief file stat system call
 * @param path for a file
 * @param buf for struct stat
 * @return success (0), fail (-1)
 */
int sys_off_stat(const char *pathname, struct stat *buf)
{
  int iret = 0;

  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  QWORD mytid = -1;

  if(pathname == NULL || buf == NULL)
    return -1;

  ch = get_offload_channel(-1);
  if(ch == NULL) {
          return -1;
  }

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys_stat, get_pa((QWORD) pathname), get_pa((QWORD) buf), 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_stat);

  return iret;
}

/**
 * @brief  file getcwd system call : get_current_dir_name - get current working directory
 * @param  buf: buffer to contain current working directory
 * @param  size: buf size
 * @return success (a pointer to a string containing the pathname of the current working directory), faile (NULL)
 */
char *sys3_off_getcwd(char *buf, size_t size)
{
  char *pret = NULL;
  int az_alloc_flag = 0;

  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;

  // check parameter
  if(buf == NULL && size == 0) 
    return (NULL);

  ch = get_offload_channel(-1);
  if(ch == NULL)
    return (NULL);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  if(buf == NULL) {
    buf = (char *) az_alloc(PAGE_SIZE_4K);
    az_alloc_flag = 1;
  }

  send_offload_message(ocq, mytid, SYSCALL_sys3_getcwd, get_pa((QWORD) buf), size, 0, 0, 0, 0);
  pret = (char *) receive_offload_message(icq, mytid, SYSCALL_sys3_getcwd);

  if(pret == NULL) {
    if(az_alloc_flag == 1)
      az_free(buf);
    return NULL;
  }

  return buf;
}

/**
 * @brief system system call :  execute a shell command
 * @param command
 * @return success (the return status of the  command), fail (-1)
 */
int sys3_off_system(char *command)
{
  int iret = -1;

  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;

  // check parameter
  if(command == NULL)
    return (-1);

  ch = get_offload_channel(-1);
  if(ch == NULL)
    return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys3_system, get_pa((QWORD) command), 0, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys3_system);

  return iret;
}

/**
 * @brief file chdir system call : change working directory
 * @param path for directory
 * @return success (0), fail (-1)
 */
int sys_off_chdir(const char *path)
{
  int iret = -1;

  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;

  // check parameter
  if(path == NULL)
    return (-1);

  ch = get_offload_channel(-1);
  if(ch == NULL)
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys_chdir, get_pa((QWORD) path), 0, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_chdir);

  return iret;
}

/**
 * @brief file opendir system call : open a directory
 * @param name for directory 
 * @return success (a pointer to the directory stream), fail (NULL)
 */
DIR *sys3_off_opendir(const char *name)
{
DIR *pret = NULL;

channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

  ch = get_offload_channel(-1);
  if(ch == NULL) {
          return NULL;
  }

  icq = ch->in;
  ocq = ch->out;

  current = get_current(); 
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys3_opendir, get_pa((QWORD) name), 0, 0, 0, 0, 0);
  pret = (DIR *)receive_offload_message(icq, mytid, SYSCALL_sys3_opendir);

  return pret;
}

/**
 * @brief file closedir system call
 * @param dirp for the directory stream
 * @return success (0), fail (-1)
 */
int sys3_off_closedir(DIR *dirp)
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

  send_offload_message(ocq, mytid, SYSCALL_sys3_closedir, (QWORD) dirp, 0, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys3_closedir);

  return iret;
}

/**
 * @brief file readdir system call : read a directory
 * @param dirp for the directory stream 
 * @return success (a pointer to a dirent structure), (NULL on end of directory), fail (NULL)
 */
struct dirent *sys3_off_readdir(DIR *dirp)
{
struct dirent *pret = NULL;
static struct dirent ret;

channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

  ch = get_offload_channel(-1);
  if(ch == NULL) {
          return NULL;
  }

  icq = ch->in;
  ocq = ch->out;

  current = get_current(); 
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys3_readdir, (QWORD) dirp, 0, 0, 0, 0, 0);
  pret = (struct dirent *) receive_offload_message(icq, mytid, SYSCALL_sys3_readdir);

  if(pret != NULL && (QWORD) pret != -1) {
    lk_memcpy(&ret, pret, sizeof(struct dirent));
    return (&ret);
  }
  else {
    return NULL;
  }
}


/**
 * @brief file rewinddir system call : reset directory stream
 * @param dirp for the position of the directory stream
 * @return none
 */
void sys3_off_rewinddir(DIR *dirp)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

  ch = get_offload_channel(-1);
  if(ch == NULL) {
          return;
  }

  icq = ch->in;
  ocq = ch->out;

  current = get_current(); 
  mytid = current->id;

  send_offload_message(ocq, mytid, SYSCALL_sys3_rewinddir, (QWORD) dirp, 0, 0, 0, 0, 0);
  receive_offload_message(icq, mytid, SYSCALL_sys3_rewinddir);
}
