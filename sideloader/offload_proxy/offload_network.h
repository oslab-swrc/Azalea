#ifndef __OFFLOAD_NETWORK_H__
#define __OFFLOAD_NETWORK_H__

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_gethostname(struct channel_struct *ch);
void sys_off_gethostbyname(struct channel_struct *ch);
void sys_off_getsockname(struct channel_struct *ch);
void sys_off_socket(struct channel_struct *ch);
void sys_off_bind(struct channel_struct *ch);
void sys_off_listen(struct channel_struct *ch);
void sys_off_connect(struct channel_struct *ch);
void sys_off_accept(struct channel_struct *ch);

#endif /* __OFFLOAD_NETWORK_H__ */
