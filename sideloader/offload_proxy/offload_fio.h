#ifndef __OFFLOAD_FIO_H__
#define __OFFLOAD_FIO_H__

#include <dirent.h>

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_open(struct channel_struct *ch);
void sys_off_creat(struct channel_struct *ch);
void sys_off_read(struct channel_struct *ch);
void sys_off_write(struct channel_struct *ch);
void sys_off_lseek(struct channel_struct *ch);
void sys_off_close(struct channel_struct *ch);
void sys_off_link(struct channel_struct *ch);
void sys_off_unlink(struct channel_struct *ch);
void sys_off_stat(struct channel_struct *ch);
void sys3_off_getcwd(struct channel_struct *ch);
void sys3_off_system(struct channel_struct *ch);
void sys_off_chdir(struct channel_struct *ch);
void sys3_off_opendir(struct channel_struct *ch);
void sys3_off_closedir(struct channel_struct *ch);
void sys3_off_readdir(struct channel_struct *ch);
void sys3_off_rewinddir(struct channel_struct *ch);
void sys3_off_opendir2(struct channel_struct *ch);

#endif /*__OFFLOAD_IO_H__*/
