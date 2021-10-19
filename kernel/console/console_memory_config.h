// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CONSOLE_MEMORY_CONFIG_H__
#define __CONSOLE_MEMORY_CONFIG_H__

#include "memory_config.h"

#define CONFIG_CONSOLE_CHANNEL_VA       (CONFIG_SHARED_MEMORY + CHANNEL_START_OFFSET)

#define CONSOLE_CHANNEL_STRUCT_VA       (CONFIG_CONSOLE_CHANNEL_VA)
#define CONSOLE_CHANNEL_INFO_VA         (CONFIG_CONSOLE_CHANNEL_VA + (PAGE_SIZE_4K * 10))
#define CONSOLE_CHANNEL_BASE_VA         (CONFIG_CONSOLE_CHANNEL_VA + (PAGE_SIZE_4K * 11))

#endif // __CONSOLE_MEMORY_CONFIG_H__
