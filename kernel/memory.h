#ifndef __MEMORY_MANAGEMENT_H__
#define __MEMORY_MANAGEMENT_H__

#include <sys/types.h>
#include "memory_config.h"
#include "sync.h"
#include "az_types.h"

#define MM_TRUE		0x01
#define MM_FALSE	0x00

#define PAGE_MIN_SIZE	(4 * 1024)

// Heap memory
#define PAGE_BITS        12 
#define PG_XD            (1L << 63)
#define PAGE_MASK        (((~0UL) << PAGE_BITS) & ~PG_XD)
#define PAGE_FLOOR(addr) ((addr)                   & PAGE_MASK)

typedef struct bitmap_struct
{
  BYTE* bitmap;
  QWORD bitmap_count;
} BITMAP;

typedef struct memory_management
{
  int max_level;	// max level of the block list
  int smallest_block;	// smallest block count
  QWORD used_size;	// size of allocated memory

  QWORD start_addr;	// start address of block pool
  QWORD end_addr;	// end address of block pool

  BYTE* block_index;	// the pointer of the block list index 
  BITMAP* bitmap;	// the pointer of BITMAP

  spinlock_t mm_spl;	// spinlock
} MM;

// functions
void free_mem_init(void);
void* az_alloc(QWORD size);
BOOL az_free(void* address);

int bitmap_size(QWORD free_mem_size);
int alloc_buddy_block(QWORD aligned_size);
QWORD align_buddy_block(QWORD size);
int block_list_index(QWORD aligned_size);
int find_free_block(int block_list_index);
BOOL free_buddy_block(int block_list_index, int block_offset);

void set_bitmap_flag(int block_list_index, int offset, BYTE flag);
BYTE get_bitmap_flag(int block_list_index, int offset);

ssize_t sys_sbrk(ssize_t incr);

#endif /* __MEMORY_MANAGEMENT_H__ */
