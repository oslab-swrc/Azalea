#ifndef __OFFLOAD_FIO_H__
#define __OFFLOAD_FIO_H__

#include <sys/types.h>
#include <sys/stat.h>

int sys_off_open(const char *path, int oflag, mode_t mode);
int sys_off_creat(const char *path, mode_t mode);
int sys_off_close(int fd);
ssize_t sys_off_read(int fd, void *buf, size_t count);
ssize_t sys_off_write(int fd, void *buf, size_t count);
off_t sys_off_lseek(int fd, off_t offset, int whence);
int sys_off_unlink(const char *path);
int sys_off_stat(const char *pathname, struct stat *buf);
char *off_getcwd(char *buf, size_t size);
int off_system(char *command);
int sys_off_chdir(const char *path);

#endif /*__OFFLOAD_FIO_H__*/
