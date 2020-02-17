#ifndef __CONSOLE_MMAP_H__
#define __CONSOLE_MMAP_H__

#include "console_channel.h"

#define CONSOLE_MAGIC           (0x5D4C3B2A)

int mmap_console_channel(channel_t *console_channel, int start_index, int opages, int ipages);
int munmap_console_channel(channel_t *console_channel);
int mmap_unikernel_memory(int start_index);
int munmap_unikernel_memory(void);

unsigned long get_va(unsigned long pa);
unsigned long get_pa_base(void);
unsigned long get_va_base(void);

#endif //__CONSOLE_MMAP_H__
