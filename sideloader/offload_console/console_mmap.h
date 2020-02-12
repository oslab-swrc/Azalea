#ifndef __CONSOLE_MMAP_H__
#define __CONSOLE_MMAP_H__

#include "console_channel.h"

#define CONSOLE_MAGIC           (0x5D4C3B2A)

int mmap_console_channel(channel_t *console_channels, int start_index, int n_console_channels, int opages, int ipages);
int munmap_console_channel(void *addr, unsigned long length);
int mmap_unikernel_memory(int node_id);
unsigned long get_va(unsigned long pa);
unsigned long get_pa_base(void);
unsigned long get_va_base(void);

#endif //__CONSOLE_MMAP_H__
