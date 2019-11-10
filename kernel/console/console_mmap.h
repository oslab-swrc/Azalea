#ifndef __CONSOLE_MMAP_H__
#define __CONSOLE_MMAP_H__

#include "offload_channel.h"

#define	PAGE_PTE_OFFSET_MASK	(PAGE_PTE_MASK + PAGE_OFFSET_MASK)

BOOL init_console_channel(void);
channel_t *get_console_channel(void);

#endif
