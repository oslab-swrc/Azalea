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

#ifndef __ssize_t_defined
//typedef long int ssize_t;
#define ssize_t long long
#define __ssize_t_defined
#endif

#ifndef __size_t_defined
//typedef unsigned long size_t;
#define size_t unsigned long long
#define __size_t_defined
#endif

#define mode_t  int
#define off_t   unsigned long

#define TRUE	1
#define FALSE	0
#ifndef NULL
#define NULL	0
#endif

#ifndef __ASSEMBLY__
#pragma pack(push, 1)
typedef struct video_char_struct {
  BYTE ch;
  BYTE attr;
} VCHAR;
#pragma pack(pop)

#endif

#endif  /* __TYPES_H__ */
