#ifndef __OFFLOAD_MMAP_H__
#define __OFFLOAD_MMAP_H__

#include "az_types.h"

#define	PAGE_PTE_OFFSET_MASK	(PAGE_PTE_MASK + PAGE_OFFSET_MASK)
unsigned long get_pa(QWORD virtual_address);

#endif
