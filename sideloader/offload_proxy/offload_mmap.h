#ifndef __OFFLOAD_MMAP_H__
#define __OFFLOAD_MMAP_H__

#include "offload_channel.h"

#define OFFLOAD_MAGIC           (0x5D4C3B2A)

int mmap_channels(channel_t *offload_channels, int n_nodes, int n_offload_channels, int opages, int ipages);
int munmap_channels(void);
unsigned long mmap_unikernels_memory(int n_nodes);
unsigned long get_va(unsigned long pa);
unsigned long get_pa_base(void);
unsigned long get_va_base(void);

#endif //__OFFLOAD_MMAP_H__
