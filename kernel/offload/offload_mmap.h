// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __OFFLOAD_MMAP_H__
#define __OFFLOAD_MMAP_H__

#include "offload_channel.h"

BOOL init_offload_channel();
channel_t *get_offload_channel(int n_requested_channel);

#endif //__OFFLOAD_MMAP_H__
