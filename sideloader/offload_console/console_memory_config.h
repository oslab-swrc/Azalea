// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CONSOLE_MEMORY_CONFIG_H__
#define __CONSOLE_MEMORY_CONFIG_H__

#include "arch.h"

#define UNIKERNELS_MEM_BASE_PA  ((unsigned long) UNIKERNEL_START << 30)
#define CONFIG_CHANNEL_PA       (SHARED_MEMORY_START + CHANNEL_START_OFFSET)

#define CONFIG_CONSOLE_CHANNEL_PA       (CONFIG_CHANNEL_PA)
#define CONSOLE_CHANNEL_STRUCT_PA       (CONFIG_CONSOLE_CHANNEL_PA)
#define CONSOLE_CHANNEL_INFO_PA         (CONFIG_CONSOLE_CHANNEL_PA + (PAGE_SIZE_4K * 10))
#define CONSOLE_CHANNEL_BASE_PA         (CONFIG_CONSOLE_CHANNEL_PA + (PAGE_SIZE_4K * 11))

#define CONSOLE_CHANNEL_NUM             (MAX_NODE_NUM) // 1 channel / node
#define CONSOLE_CHANNEL_CQ_ELEMENT_NUM  (4)

#endif
// __CONSOLE_MEMORY_CONFIG_H__
