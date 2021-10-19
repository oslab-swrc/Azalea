// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CONSOLE_MMAP_H__
#define __CONSOLE_MMAP_H__

#include "offload_channel.h"
#include "memory_config.h"
#include "az_types.h"

#define CONSOLE_CHANNEL_CQ_ELEMENT_NUM  (4)
#define CONSOLE_MAGIC   (0x5D4C3B2A)

BOOL init_console_channel(void);
channel_t *get_console_channel(void);

#endif //__CONSOLE_MMAP_H__
