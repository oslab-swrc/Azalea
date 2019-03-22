#ifndef __OFFLOAD_IO_H__
#define __OFFLOAD_IO_H__

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/uio.h>

#include <sys/types.h>

int sys_off_open(const char *path, int oflag, mode_t mode);
int sys_off_creat(const char *path, mode_t mode);
int sys_off_close(int fd);
ssize_t sys_off_read(int fd, void *buf, size_t count);
ssize_t sys_off_write(int fd, void *buf, size_t count);
#if 0
int sys_off_access(const char *pathname, int mode);
//int sys_off_stat(const char *path, struct stat *buf);
//int sys_off_lstat(const char *path, struct stat *buf);
//int sys_off_fstat(int fd, struct stat *buf);

off_t sys_off_lseek(int fd, off_t offset, int whence);
ssize_t sys_off_readlink(const char *path, char *buf, size_t bufsiz);


ssize_t sys_off_pread64(int fd, void *buf, size_t count, off_t offset);
ssize_t sys_off_pread(int fd, void *buf, size_t count, off_t offset);
//ssize_t sys_off_readv(int fd, const struct iovec *iov, int iovcnt);
//ssize_t sys_off_preadv(int fd, const struct iovec *iov, int iovcnt,  off_t offset);

ssize_t sys_off_pwrite64(int fd, const void *buf, size_t count, off_t offset);
ssize_t sys_off_pwrite(int fd, const void *buf, size_t count, off_t offset);
//ssize_t sys_off_writev(int fd, const struct iovec *iov, int iovcnt);
//ssize_t sys_off_pwritev(int fd, const struct iovec *iov, int iovcnt,  off_t offset);
#endif

#endif
