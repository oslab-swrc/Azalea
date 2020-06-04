#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>

#include "offload_channel.h"
#include "offload_fio.h"
#include "offload_message.h"
#include "offload_mmap.h"
#include "offload_thread_pool.h"

//#define DEBUG
//#define IOVEC_RW

/**
 * @brief execute open system call
 * @param channel 
 * @return none
 */
void sys_off_open(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;
  int  oflag = 0;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  oflag = (int) in_pkt->param2;
  mode = (mode_t) in_pkt->param3;

#ifdef DEBUG
  printf("\nopen system call(pa): path=%lx, flags=%d, mode=%d", in_pkt->param1, (int) in_pkt->param2, (int) in_pkt->param3);
  printf("\nopen system call: path=%s, flags=%d, mode=%d", path, oflag, mode);
#endif

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
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute creat system call
 * @param channel 
 * @return none
 */
void sys_off_creat(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;
  mode_t mode = 0;

  int  tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);
  mode = (mode_t) in_pkt->param2;

#ifdef DEBUG
  printf("\ncreat system call: path=%s, mode=%d", path, mode);
#endif

  // execute create
  iret = creat(path, mode);

  if(iret == -1)
	fprintf(stderr, "FIO CREAT: %s, %s\n", strerror(errno), path);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
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
void sys_off_read(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  ssize_t sret = 0;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  pa = (unsigned long) in_pkt->param2;
  count = (int) in_pkt->param3;

  // execute read

#ifdef IOVEC_RW
  sret = do_sys_off_read_v(fd, pa, count);
#else
  sret = read(fd, (void *) get_va(pa), (size_t) count);
#endif

  // check error
  if(sret  == -1)
	fprintf(stderr, "FIO READ: %s, fd: %d, pa: %lx\n", strerror(errno), fd, pa);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) sret);
  pthread_mutex_unlock(&ch->mutex);
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
void sys_off_write(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  ssize_t sret = -1;

  int fd = 0;
  unsigned long pa = 0;
  int count = 0;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  pa = (unsigned long) in_pkt->param2;
  count = (int) in_pkt->param3;

  // execute write
#ifdef IOVEC_RW
  sret = do_sys_off_write_v(fd, pa, count);
#else
  sret = write(fd, (void *) get_va(pa), (size_t) count);
#endif

  // check error
  if(sret == -1)
	fprintf(stderr, "FIO WRITE: %s, fd: %d, pa: %lx\n", strerror(errno), fd, pa);

  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) sret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute close system call
 * close()  closes  a  file descriptor.
 * @param channel 
 * @return none
 */
void sys_off_close(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  int  fd = 0;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;


  if(fd > 2) {
    // execute close
    iret = close(fd);
 
    // check error
    if(iret == -1)
      fprintf(stderr, "FIO CLOSE: %s, %d\n", strerror(errno), fd);
  }
  else {
    iret = -1;
  }

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}


/**
 * @brief execute lseek system call
 * lseek() function repositions the offset of the open file.
 * @param channel 
 * @return none
 */
void sys_off_lseek(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  off_t  oret = -1;

  int  fd = 0;
  off_t offset = 0;
  int whence = 0;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  fd = (int) in_pkt->param1;
  offset = (off_t) in_pkt->param2;
  whence = (int) in_pkt->param3;

  // execute lseek
  oret = lseek(fd, offset, whence);

  // check error
  if(oret == -1)
        fprintf(stderr, "FIO LSEEK: %s, %d\n", strerror(errno), fd);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) oret);
  pthread_mutex_unlock(&ch->mutex);
}


/**
 * @brief execute link system call
 * link() creates a new link (also known as a hard link) to an existing file.
 * @param channel 
 * @return none
 */
void sys_off_link(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *oldpath = NULL;;
  char *newpath = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  oldpath = (char *) get_va(in_pkt->param1);
  newpath = (char *) get_va(in_pkt->param2);

  // execute link
  iret = link(oldpath, newpath);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO LINK: %s, %s %s\n", strerror(errno), oldpath, newpath);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}


/**
 * @brief execute unlink system call
 * unlink()  deletes a name from the file system.
 * @param channel 
 * @return none
 */
void sys_off_unlink(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *path = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);

  // execute unlink
  if(access(path, F_OK) == 0) {
    iret = unlink(path);
    // check error
    if(iret == -1)
      fprintf(stderr, "FIO UNLINK: %s, %s\n", strerror(errno), path);
  }
  else {
    iret = 0;
  }


  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute stat system call 
 * @param channel 
 * @return none
 */
void sys_off_stat(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int  iret = -1;

  char *pathname = NULL;;
  struct stat *buf = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  pathname = (char *) get_va(in_pkt->param1);
  buf = (struct stat *) get_va(in_pkt->param2);

  // execute unlink
  iret = stat(pathname, buf);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO STAT: %s, %s\n", strerror(errno), pathname);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute getcwd system call 
 * @param channel 
 * @return none
 */
void sys3_off_getcwd(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  char *pret = NULL;

  char *buf = NULL;;
  size_t size = 0;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  buf = (char *) get_va(in_pkt->param1);
  size = (size_t) in_pkt->param2;

  // execute unlink
  pret = getcwd(buf, size);

  // check error
  if(pret == NULL)
    fprintf(stderr, "FIO GETCWd: %s, %s\n", strerror(errno), buf);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute system system call
 * @param channel 
 * @return none
 */
void sys3_off_system(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  char *command = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  command = (char *) get_va(in_pkt->param1);

  // execute unlink
  iret = system(command);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO SYSTEM: %s, %s\n", strerror(errno), command);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute chdir system call
 * @param channel 
 * @return none
 */
void sys_off_chdir(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  char *path = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  path = (char *) get_va(in_pkt->param1);

  // execute unlink
  iret = chdir(path);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO CHDIR: %s, %s\n", strerror(errno), path);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute opendir system call
 * @param channel 
 * @return none
 */
void sys3_off_opendir(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  DIR *pret = NULL;

  char *name = NULL;;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  name = (char *) get_va(in_pkt->param1);

  // execute opendir 
  pret = opendir(name);

  // check error
  if(pret == NULL)
    fprintf(stderr, "FIO OPENDIR: %s, NULL\n", strerror(errno));

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
  pthread_mutex_unlock(&ch->mutex);
}


/**
 * @brief execute closedir system call
 * @param channel 
 * @return none
 */
void sys3_off_closedir(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int iret = -1;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  // execute closedir 
  iret = closedir(dirp);

  // check error
  if(iret == -1)
    fprintf(stderr, "FIO CLOSEDIR: %s\n", strerror(errno));

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) iret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute readdir system call
 * @param channel 
 * @return none
 */
void sys3_off_readdir(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  struct dirent *pret = NULL;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  //printf("readdir(l): readdir: dirp: %lx \n", dirp);

  // execute readdir 
  errno = 0;
  pret = readdir(dirp);
  //printf("readdir(l): readdir: direntp: %lx \n", pret);

  if(pret == NULL && errno != 0) {
    fprintf(stderr, "FIO READDIR: %s\n", strerror(errno));
    pret = (struct dirent *) -1;
  }

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) pret);
  pthread_mutex_unlock(&ch->mutex);
}

/**
 * @brief execute rewinddir system call
 * @param channel 
 * @return none
 */
void sys3_off_rewinddir(job_args_t *job_args)
{
  struct circular_queue *out_cq = NULL;
  io_packet_t *in_pkt = NULL;

  DIR *dirp = NULL;

  int tid = 0;
  int  offload_function_type = 0;

  struct channel_struct *ch;
  ch = job_args->ch;
  out_cq = job_args->ch->out_cq;
  in_pkt = (io_packet_t *) &job_args->pkt;

  tid = (int) in_pkt->tid;
  offload_function_type = (int) in_pkt->io_function_type;

  dirp = (DIR *) in_pkt->param1;

  // execute closedir 
  rewinddir(dirp);

  // retrun ret
  pthread_mutex_lock(&ch->mutex);
  send_offload_message(out_cq, tid, offload_function_type, (unsigned long) 0);
  pthread_mutex_unlock(&ch->mutex);
}
