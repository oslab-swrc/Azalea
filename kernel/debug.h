// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "az_types.h"

void debug_info(char *func, QWORD v, QWORD v2);
void debug_info_xy(int x, int y, char *func, QWORD v, QWORD v2);
void debug_halt(char *func, QWORD v);
void debug_niy(char *func, QWORD v);

#endif  /* __DEBUG_H__ */
