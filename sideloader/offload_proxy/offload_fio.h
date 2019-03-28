#ifndef __OFFLOAD_IO_H__
#define __OFFLOAD_IO_H__

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_open(struct channel_struct *ch);
void sys_off_creat(struct channel_struct *ch);
void sys_off_read(struct channel_struct *ch);
void sys_off_write(struct channel_struct *ch);
void sys_off_lseek(struct channel_struct *ch);
void sys_off_close(struct channel_struct *ch);
void sys_off_unlink(struct channel_struct *ch);

#endif
