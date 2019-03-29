#ifndef __OFFLOAD_MMAP_H__
#define __OFFLOAD_MMAP_H__

#include "offload_channel.h"

int mmap_channels(channel_t *io_channels, int n_io_channels, int opages, int ipages);
int munmap_channels(channel_t *io_channels, int n_io_channels);
int mmap_unikernels_memory();
unsigned long get_va(unsigned long pa);
unsigned long get_pa_base();
unsigned long get_va_base();

#endif
