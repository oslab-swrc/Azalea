#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "offload_channel.h"
#include "offload_fio.h"
#include "offload_message.h"
#include "offload_mmap.h"

//#define DEBUG

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
  printf("\nopen system call(pa): path=%lx, flags=%d, mode=%d", in_pkt->param1, (int) in_pkt->param2, (int) in_pkt->param3);
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
	fprintf(stderr, "FIO OPEN: %s, %s\n", strerror(errno), path);

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
	fprintf(stderr, "FIO CREAT: %s, %s\n", strerror(errno), path);

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

	  //next iov
	  iov++;
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
	fprintf(stderr, "FIO READ: %s, %d\n", strerror(errno), fd);

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

	  //next iov
	  iov++;
	}

	//fprintf(stderr, "\nwrite : fd=%d write bytes=%d", fd, writebytes);
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
	fprintf(stderr, "FIO WRITE: %s, %d\n", strerror(errno), fd);

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
	fprintf(stderr, "FIO CLOSE: %s, %d\n", strerror(errno), fd);

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
        fprintf(stderr, "FIO LSEEK: %s, %d\n", strerror(errno), fd);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) oret);
}


/**
 * @brief execute link system call
 * link() creates a new link (also known as a hard link) to an existing file.
 * @param channel 
 * @return none
 */
void sys_off_link(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *oldpath = NULL;;
  char *newpath = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  oldpath = (char *) get_va(in_pkt->param1);
  newpath = (char *) get_va(in_pkt->param2);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute link
  iret = link(oldpath, newpath);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO LINK: %s, %s %s\n", strerror(errno), oldpath, newpath);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
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
    fprintf(stderr, "FIO UNLINK: %s, %s\n", strerror(errno), path);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute stat system call 
 * @param channel 
 * @return none
 */
void sys_off_stat(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *pathname = NULL;;
  struct stat *buf = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  pathname = (char *) get_va(in_pkt->param1);
  buf = (struct stat *) get_va(in_pkt->param2);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute unlink
  iret = stat(pathname, buf);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO STAT: %s, %s\n", strerror(errno), pathname);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute getcwd system call 
 * @param channel 
 * @return none
 */
void sys3_off_getcwd(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  char *pret = NULL;

  char *buf = NULL;;
  size_t size = 0;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  buf = (char *) get_va(in_pkt->param1);
  size = (size_t) in_pkt->param2;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute unlink
  pret = getcwd(buf, size);

  // check error
  if(pret == NULL)
    fprintf(stderr, "FIO GETCWd: %s, %s\n", strerror(errno), buf);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
}

/**
 * @brief execute system system call
 * @param channel 
 * @return none
 */
void sys3_off_system(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  char *command = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  command = (char *) get_va(in_pkt->param1);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute unlink
  iret = system(command);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO SYSTEM: %s, %s\n", strerror(errno), command);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute chdir system call
 * @param channel 
 * @return none
 */
void sys_off_chdir(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

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
  iret = chdir(path);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO CHDIR: %s, %s\n", strerror(errno), path);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute opendir system call
 * @param channel 
 * @return none
 */
void sys3_off_opendir(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  DIR *pret = NULL;

  char *name = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  name = (char *) get_va(in_pkt->param1);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute opendir 
  pret = opendir(name);

  // check error
  if(pret == NULL)
    fprintf(stderr, "FIO OPENDIR: %s, NULL\n", strerror(errno));

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
}


/**
 * @brief execute closedir system call
 * @param channel 
 * @return none
 */
void sys3_off_closedir(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute closedir 
  iret = closedir(dirp);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO CLOSEDIR: %s\n", strerror(errno));

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
}

/**
 * @brief execute readdir system call
 * @param channel 
 * @return none
 */
void sys3_off_readdir(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  struct dirent *pret = NULL;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  //printf("readdir(l): readdir: dirp: %lx \n", dirp);

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute readdir 
  errno = 0;
  pret = readdir(dirp);
  //printf("readdir(l): readdir: direntp: %lx \n", pret);

  if(pret == NULL && errno != 0) {
    fprintf(stderr, "FIO READDIR: %s\n", strerror(errno));
    pret = (struct dirent *) -1;
  }

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
}

/**
 * @brief execute rewinddir system call
 * @param channel 
 * @return none
 */
void sys3_off_rewinddir(struct channel_struct *ch)
{
  struct circular_queue *in_cq = NULL;
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  in_cq = ch->in_cq;
  out_cq = ch->out_cq;
  in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  // empty in_cq
  in_cq->tail = (in_cq->tail + 1) % in_cq->size;

  // execute closedir 
  rewinddir(dirp);

  // retrun ret
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) 0);
}
