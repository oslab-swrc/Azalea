#ifndef __CONSOLE_MMAP_H__
#define __CONSOLE_MMAP_H__

#include "console_channel.h"

int mmap_console_channels(channel_t *console_channels, int node_id, int n_console_channels, int opages, int ipages);
int munmap_console_channels(channel_t *console_channels, int n_console_channels);
int mmap_unikernel_memory(int node_id);
unsigned long get_va(unsigned long pa);
unsigned long get_pa_base(void);
unsigned long get_va_base(void);

#endif
