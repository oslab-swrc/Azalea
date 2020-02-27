#ifndef __OFFLOAD_FIO_H__
#define __OFFLOAD_FIO_H__

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_open(thread_job_t *job);
void sys_off_creat(thread_job_t *job);
void sys_off_read(thread_job_t *job);
void sys_off_write(thread_job_t *job);
void sys_off_lseek(thread_job_t *job);
void sys_off_close(thread_job_t *job);
void sys_off_link(thread_job_t *job);
void sys_off_unlink(thread_job_t *job);
void sys_off_stat(thread_job_t *job);
void sys3_off_getcwd(thread_job_t *job);
void sys3_off_system(thread_job_t *job);
void sys_off_chdir(thread_job_t *job);
void sys3_off_opendir(thread_job_t *job);
void sys3_off_closedir(thread_job_t *job);
void sys3_off_readdir(thread_job_t *job);
void sys3_off_rewinddir(thread_job_t *job);
void sys3_off_opendir2(thread_job_t *job);

#endif /*__OFFLOAD_IO_H__*/
