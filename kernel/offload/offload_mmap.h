#ifndef __OFFLOAD_MMAP_H__
#define __OFFLOAD_MMAP_H__

#include "offload_channel.h"

BOOL init_offload_channel();
channel_t *get_offload_channel(int n_requested_channel);

#endif
