#ifndef __OFFLOAD_FIO_H__
#define __OFFLOAD_FIO_H__

#include "offload_channel.h"
#include "offload_message.h"
#include "offload_thread_pool.h"

void sys_off_open(job_args_t *job_args);
void sys_off_creat(job_args_t *job_args);
void sys_off_read(job_args_t *job_args);
void sys_off_write(job_args_t *job_args);
void sys_off_lseek(job_args_t *job_args);
void sys_off_close(job_args_t *job_args);
void sys_off_link(job_args_t *job_args);
void sys_off_unlink(job_args_t *job_args);
void sys_off_stat(job_args_t *job_args);
void sys3_off_getcwd(job_args_t *job_args);
void sys3_off_system(job_args_t *job_args);
void sys_off_chdir(job_args_t *job_args);
void sys3_off_opendir(job_args_t *job_args);
void sys3_off_closedir(job_args_t *job_args);
void sys3_off_readdir(job_args_t *job_args);
void sys3_off_rewinddir(job_args_t *job_args);
void sys3_off_opendir2(job_args_t *job_args);

#endif /*__OFFLOAD_IO_H__*/
