// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __MAP_H__
#define __MAP_H__

#include "az_types.h"
#include "page.h"
#include "sync.h"

spinlock_t map_lock;

void map_init(void);
BOOL lk_map(QWORD virtual_address, QWORD physical_address, int page_size, QWORD attr);
BOOL lk_unmap(QWORD virtual_address);
int free_page_table(PT_ENTRY* p);

#endif  /* __MAP_H__ */
