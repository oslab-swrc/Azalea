// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __SYSTEMCALL_H__
#define __SYSTEMCALL_H__

#include "az_types.h"
#include "assemblyutility.h"

static inline void set_kernel_gs_base(QWORD kernel_pointer)
{
  QWORD rdx, rax;

  // IA32_KERNELGSBASE 
  rdx = ((QWORD) kernel_pointer) >> 32;
  rax = (kernel_pointer & 0xFFFFFFFF);

  write_msr(0xC0000102, rdx, rax);
}

void systemcall_entrypoint(void);
QWORD process_systemcall(QWORD param1, QWORD param2, QWORD param3,
                         QWORD param4, QWORD param5, QWORD param6,
                         QWORD no);
void systemcall_init(void);

#endif                          // __SYSTEMCALL_H__
