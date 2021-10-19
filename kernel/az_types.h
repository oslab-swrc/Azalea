// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __TYPES_H__
#define __TYPES_H__

#ifndef __BYTE_defined
#define BYTE	unsigned char
#define __BYTE_defined
#endif

#ifndef __WORD_defined
#define WORD	unsigned short
#define __WORD_defined
#endif

#ifndef __DWORD_defined
#define DWORD	unsigned int
#define __DWORD_defined
#endif

#ifndef __QWORD_defined
#define QWORD	unsigned long
#define __QWORD_defined
#endif

#ifndef __BOOL_defined
#define BOOL	unsigned char
#define __BOOL_defined
#endif

#define tid_t	int

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL	0
#endif

#endif  /* __TYPES_H__ */
