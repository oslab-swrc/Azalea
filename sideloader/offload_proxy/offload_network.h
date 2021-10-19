// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __OFFLOAD_NETWORK_H__
#define __OFFLOAD_NETWORK_H__

#include "offload_channel.h"
#include "offload_message.h"
#include "offload_thread_pool.h"

void sys_off_gethostname(job_args_t *job_args);
void sys_off_gethostbyname(job_args_t *job_args);
void sys_off_getsockname(job_args_t *job_args);
void sys_off_socket(job_args_t *job_args);
void sys_off_bind(job_args_t *job_args);
void sys_off_listen(job_args_t *job_args);
void sys_off_connect(job_args_t *job_args);
void sys_off_accept(job_args_t *job_args);

#endif /* __OFFLOAD_NETWORK_H__ */
