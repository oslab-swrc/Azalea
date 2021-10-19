// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __OFFLOAD_PAGE_H__
#define __OFFLOAD_PAGE_H__

#include <sys/uio.h>

#include "az_types.h"

#define	PAGE_PTE_OFFSET_MASK	(PAGE_PTE_MASK + PAGE_OFFSET_MASK)

unsigned long get_pa(QWORD virtual_address);
int get_iovec(void *buf, size_t count, struct iovec *iov, int *iovcnt);

#endif
