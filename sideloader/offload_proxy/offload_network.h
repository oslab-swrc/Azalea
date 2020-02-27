#ifndef __OFFLOAD_NETWORK_H__
#define __OFFLOAD_NETWORK_H__

#include "offload_channel.h"
#include "offload_message.h"

void sys_off_gethostname(thread_job_t *job);
void sys_off_gethostbyname(thread_job_t *job);
void sys_off_getsockname(thread_job_t *job);
void sys_off_socket(thread_job_t *job);
void sys_off_bind(thread_job_t *job);
void sys_off_listen(thread_job_t *job);
void sys_off_connect(thread_job_t *job);
void sys_off_accept(thread_job_t *job);

#endif /* __OFFLOAD_NETWORK_H__ */
