#ifndef __SHM_MM_H__
#define __SHM_MM_H__

#include <sys/ipc.h>
#include <sys/lock.h>
#include <sys/shm.h>

#include "arch.h"
#include "memory.h"
#include "page.h"

#define MM_TRUE		0x01
#define MM_FALSE	0x00

#define PAGE_MIN_SIZE	(4 * 1024)

#define PAGE_BITS        12 
#define PG_XD            (1L << 63)
#define PAGE_MASK        (((~0UL) << PAGE_BITS) & ~PG_XD)
#define PAGE_FLOOR(addr) ((addr)                   & PAGE_MASK)

void shm_free_mem_init(void);
void* shm_alloc(QWORD size);
BOOL shm_free( void* address);

int shm_bitmap_size(QWORD free_mem_size);
int shm_alloc_buddy_block(QWORD aligned_size);
QWORD shm_align_buddy_block(QWORD size);
int shm_block_list_index(QWORD aligned_size);
int shm_find_free_block(int block_list_index);
BOOL shm_free_buddy_block(int block_list_index, int block_offset);
void shm_set_bitmap_flag(int block_list_index, int offset, BYTE flag);
BYTE shm_get_bitmap_flag(int block_list_index, int offset);
size_t shm_get_buddy_block_size(size_t size);

#endif /* __SHM_MM_H__ */
