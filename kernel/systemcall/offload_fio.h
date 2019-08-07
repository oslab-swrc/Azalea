#ifndef __OFFLOAD_FIO_H__
#define __OFFLOAD_FIO_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

int sys_off_open(const char *path, int oflag, mode_t mode);
int sys_off_creat(const char *path, mode_t mode);
int sys_off_close(int fd);
ssize_t sys_off_read(int fd, void *buf, size_t count);
ssize_t sys_off_write(int fd, void *buf, size_t count);
off_t sys_off_lseek(int fd, off_t offset, int whence);
int sys_off_link(const char *oldpath, const char *newpath);
int sys_off_unlink(const char *path);
int sys_off_stat(const char *pathname, struct stat *buf);
char *sys3_off_getcwd(char *buf, size_t size);
int sys3_off_system(char *command);
int sys_off_chdir(const char *path);
DIR *sys3_off_opendir(const char *name);
int sys3_off_closedir(DIR *dirp);
struct dirent *sys3_off_readdir(DIR *dirp);
void sys3_off_rewinddir(DIR *dirp);
DIR *sys3_off_opendir2(const char *name, DIR **pdir);

#endif /*__OFFLOAD_FIO_H__*/
