// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __SHM_MM_CONFIG_H__
#define __SHM_MM_CONFIG_H__

#include "arch.h"
#include "page.h"

#define CONFIG_SHM_SHARED_MEMORY        ((QWORD) CONFIG_SHARED_MEMORY + (QWORD) IPC_START_OFFSET)
#define CONFIG_SHM_SHARED_MEMORY_SIZE   IPC_SIZE
#define SHM_INFO_S_VA			CONFIG_SHM_SHARED_MEMORY

#endif /* __SHM_MM_CONFIG_H__ */
