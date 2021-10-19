// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __CONSOLE_FUNCTION_H__
#define __CONSOLE_FUNCTION_H__

#include <sys/types.h>

#define	CONSOLE_PRINT	1
#define	CONSOLE_WRITE	2
#define	CONSOLE_GETCH	3
#define	CONSOLE_EXIT	(-9)

int cs_boot_msg_print(int yloc);
int cs_printf(const char *parameter, ...);

ssize_t cs_write(int fd, void *buf, size_t count);
int cs_puts(void *buf);
int cs_exit(void);

#endif //__CONSOLE_FUNCTION_H__
