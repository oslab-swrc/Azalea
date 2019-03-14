#include "memory.h"
#include "memory_config.h"
#include "console.h"
#include "page.h"
#include "az_types.h"
#include "utility.h"

static MM g_free_memory;
static QWORD g_alloc_count;
static QWORD g_free_count;

QWORD g_memory_start;
QWORD g_memory_end;

static QWORD heap_end;

static spinlock_t heap_lock;

/**
 * Initlaize the free memory area    
 */
void free_mem_init(void)
{
  int i = 0, j = 0;
  BYTE* cur_bitmap = NULL;
  int block_count = 0, bitmap = 0;

  QWORD free_mem_start = va(g_memory_start + CONFIG_KERNEL_SIZE);  // free memory start addr
  QWORD free_mem_end = va(g_memory_end);  // free memory end addr 
  QWORD free_mem_size = free_mem_end - free_mem_start;	// free memory size

  bitmap = bitmap_size(free_mem_size);	// bitmap size (block list index + bitmap address + bitmap)
  bitmap = ((bitmap + (512 - 1)) & ~(512 - 1));

  g_free_memory.smallest_block = (free_mem_size / PAGE_MIN_SIZE) - bitmap;

  // calculate the max level of the block list
  for (i=0; (g_free_memory.smallest_block>>i) > 0; i++)
    ;
  g_free_memory.max_level = i;

  // Initialize the block list index areas to 0xFF
  g_free_memory.block_index = (BYTE*) free_mem_start;

  for (i=0; i<g_free_memory.smallest_block; i++)
    g_free_memory.block_index[i] = 0xFF;

  // the base address of the bitmap
  g_free_memory.bitmap = (BITMAP*) (free_mem_start + (sizeof(BYTE) * g_free_memory.smallest_block));
  // real address of bitmap
  cur_bitmap = ((BYTE*) g_free_memory.bitmap) + (sizeof(BITMAP) * g_free_memory.max_level);

  // Initialize the bitmap - set to 0x00 except the top level and trash block
  for (j=0; j<g_free_memory.max_level; j++) {
    g_free_memory.bitmap[j].bitmap = cur_bitmap;
    g_free_memory.bitmap[j].bitmap_count = 0;
    block_count = g_free_memory.smallest_block >> j;

    // 8 blocks can be merged into upper level
    for (i=0; i<block_count / 8; i++) {
      *cur_bitmap = 0x00;
      cur_bitmap++;
    }

    // if there is less than 8 the blocks area
    if ((block_count % 8) != 0) {
      *cur_bitmap = 0x00;
      // if left block is odd number, it cannot be merged - trash block
      i = block_count % 8;
      if ((i % 2) == 1) {
        *cur_bitmap |= (MM_TRUE << (i - 1));
        g_free_memory.bitmap[j].bitmap_count = 1;
      }
      cur_bitmap++;
    }
  }

  // setting the start and end address of the block pool
  g_free_memory.start_addr = free_mem_start + bitmap * PAGE_MIN_SIZE;
  g_free_memory.end_addr = free_mem_end;
  g_free_memory.used_size = 0;

  // Initialize spinlock for free memory management
  spinlock_init(&(g_free_memory.mm_spl));

  // Initialize heap
  heap_end = HEAP_START;		// ??

  // Initialize spinlock for heap memory management
  spinlock_init(&heap_lock);
}

/**
 *  Allocate the input size memory
 */
void* alloc(QWORD size)
{
  QWORD aligned_size = 0;
  QWORD rel_address = 0;
  long bl_offset = -1;
  int array_offset = -1;
  int bl_index = -1;
  char *ret_addr = NULL;

  spinlock_lock(&(g_free_memory.mm_spl));

  aligned_size = align_buddy_block(size);
  if (aligned_size == 0) {
     spinlock_unlock(&(g_free_memory.mm_spl));
    return 0;
  }

  // if not enough memory, return NULL
  if (g_free_memory.start_addr + g_free_memory.used_size + aligned_size > g_free_memory.end_addr) {
    spinlock_unlock(&(g_free_memory.mm_spl));
    return 0;
  }

  // allocate buddy block and get index
  bl_offset = alloc_buddy_block(aligned_size);
  if (bl_offset == -1) {
    spinlock_unlock(&(g_free_memory.mm_spl));
    return 0;
  }

  bl_index = block_list_index(aligned_size);

  // save the block list index into the g_free_memory (It is used when deallocation)
  rel_address = aligned_size * bl_offset;
  array_offset = rel_address / PAGE_MIN_SIZE;

  g_free_memory.block_index[array_offset] = (BYTE) bl_index;
  g_free_memory.used_size += aligned_size;

  ret_addr = (char*) (rel_address + g_free_memory.start_addr);

  // initialize the allocated memory to 0
  lk_memset(ret_addr, 0, aligned_size);

  g_alloc_count++;
  lk_print_xy(50, 22, "ac: %d, fc: %d   ", g_alloc_count, g_free_count);
  spinlock_unlock(&(g_free_memory.mm_spl));
  return ret_addr;
}

/**
 * free the input address' memory
 *   - input: virtual address
 */
BOOL free( void* address )
{
  QWORD rel_address = 0;
  int array_offset = -1;
  QWORD block_size = 0;
  int block_list_index = -1;
  int bitmap_offset = -1;

  spinlock_lock(&(g_free_memory.mm_spl));

  if (address == NULL) {
    spinlock_unlock(&(g_free_memory.mm_spl));
    return FALSE;
  }

  // check thd assinged block size by converse the address
  rel_address = ( ( QWORD ) address ) - g_free_memory.start_addr;
  array_offset = rel_address / PAGE_MIN_SIZE;

  // if not assigned, return FALSE
  if (g_free_memory.block_index[array_offset] == 0xFF) {
    spinlock_unlock(&(g_free_memory.mm_spl));
    return FALSE;
  }

  // initialize the index of the block list
  // search the block list contained the assigned block
  block_list_index = (int) g_free_memory.block_index[array_offset];
  g_free_memory.block_index[array_offset] = 0xFF;

  // count the assgined block size
  block_size = PAGE_MIN_SIZE << block_list_index;

  // free the block by the block offset in the block list
  if(block_size == 0) return FALSE;
  bitmap_offset = rel_address / block_size;
  if (free_buddy_block(block_list_index, bitmap_offset) == TRUE) {
    g_free_memory.used_size -= block_size;

    g_free_count++;
    lk_print_xy(50, 22, "ac: %d, fc: %d   ", g_alloc_count, g_free_count);

    spinlock_unlock(&(g_free_memory.mm_spl));
    return TRUE;
  }

  spinlock_unlock(&(g_free_memory.mm_spl));
  return FALSE;
}

/**
 * Calculate the necessary space for saving metadata of free memory
 */
int bitmap_size(QWORD free_mem_size)
{
  long smallest_block_size = 0;
  DWORD block_index = 0;
  DWORD bitmap_size = 0;
  long i = 0;

  // smallest block size
  smallest_block_size = free_mem_size / PAGE_MIN_SIZE;
  // count the required spce for the index of the block list
  block_index = smallest_block_size * sizeof(BYTE);

  // calculate the necessary space for bitmap
  bitmap_size = 0;
  for (i=0; (smallest_block_size >> i) > 0; i++) {
    // for bitmap structure
    bitmap_size += sizeof( BITMAP );
    // for block list
    bitmap_size += ((smallest_block_size >> i) + 7) / 8;
  }

  // return the required mem size for bitmap
  return (block_index + bitmap_size + PAGE_MIN_SIZE - 1) / PAGE_MIN_SIZE;
}

/**
 * Buddy block allocation
 */
int alloc_buddy_block(QWORD aligned_size)
{
  int bl_index = -1, free_offset = -1;
  int i = 0;

  // get block list index satisfied required block size
  bl_index = block_list_index(aligned_size);
  if (bl_index == -1)
    return -1;

//  spinlock_lock(&(g_free_memory.mm_spl));

  // find the block until found from bl_index to max_level
  for (i=bl_index; i<g_free_memory.max_level; i++) {

     // check the bitmap of the block list
    free_offset = find_free_block(i);
    if (free_offset != -1)
      break;
  }

  // if not fount, fail
  if (free_offset == -1) {
//    spinlock_unlock(&(g_free_memory.mm_spl));

    return -1;
  }

  // if found, set flag in bitmap
  set_bitmap_flag(i, free_offset, MM_FALSE);

  // if the block is found in upper block, split it
  // set the left block empty and the right block exist, from i to bl_index
  if (i > bl_index) {

    for (i=i-1; i>=bl_index; i--) {
      // set the left block empty
      set_bitmap_flag(i, free_offset * 2, MM_FALSE);
      // set the rignt block exist
      set_bitmap_flag(i, free_offset * 2 + 1, MM_TRUE);
      // split the left block again
      free_offset = free_offset * 2;
    }
  }

//  spinlock_unlock(&(g_free_memory.mm_spl));

  return free_offset;
}

/**
 * Return the close buddy block size
 */
QWORD align_buddy_block(QWORD size)
{
  long i = 0;

  for (i=0; i<g_free_memory.max_level; i++) {
    if (size <= (PAGE_MIN_SIZE << i))
      return (PAGE_MIN_SIZE << i);
  }

  return 0;
}

/**
 * Return the block list index closed to the aligned_size 
 */
int block_list_index(QWORD aligned_size)
{
  int i = 0;

  for (i=0; i<g_free_memory.max_level; i++)
    if (aligned_size <= (PAGE_MIN_SIZE << i))
      return i;

  return -1;
}

/**
 * Search the bitmap of the block list
 * if found, return the offset of the block
 */
int find_free_block(int block_list_index)
{
  int i = 0, max_count = 0;
  BYTE* ptr_bitmap = NULL;
  QWORD* bitmap64 = NULL;

  // if no data in the bitmap, return -1
  if (g_free_memory.bitmap[block_list_index].bitmap_count == 0)
    return -1;

  // Count the total number of the block in the block list, the check the bitmap
  max_count = g_free_memory.smallest_block >> block_list_index;
  ptr_bitmap = g_free_memory.bitmap[block_list_index].bitmap;

  for (i=0; i<max_count; ) {
    // check whether the bit set by 64 bits (QWORD = 8*8 bits)
    if (((max_count - i) / 64) > 0) {
      bitmap64 = (QWORD*) &(ptr_bitmap[i / 8]);

      // if all is set 0, exclude them
      if (*bitmap64 == 0) {
        i += 64;
        continue;
      }
    }

    if ((ptr_bitmap[i / 8] & (MM_TRUE << (i % 8))) != 0)
      return i;

    i++;
  }
  return -1;
}

/**
 * Free buddy block
 */
BOOL free_buddy_block(int block_list_index, int block_offset)
{
  int buddy_block_offset = -1;
  int i = 0;
  BOOL flag = FALSE;

//  spinlock_lock(&(g_free_memory.mm_spl));

  // find adjoin block to the end of block list
  for (i=block_list_index; i<g_free_memory.max_level; i++) {
    // set current block exist
    set_bitmap_flag(i, block_offset, MM_TRUE);

    // if the offset of block is odd number, check the block of even number
    // if exist, merge them
    if ((block_offset % 2) == 0)
      buddy_block_offset = block_offset + 1;
    else
      buddy_block_offset = block_offset - 1;

    flag = get_bitmap_flag(i, buddy_block_offset);

    // if the block is empty, EXIT
    if (flag == MM_FALSE)
      break;

    // merge the blocks
    // set current and found block empty, then move to the upper block
    set_bitmap_flag(i, buddy_block_offset, MM_FALSE);
    set_bitmap_flag(i, block_offset, MM_FALSE);

    // change the offset of the upper block list's
    block_offset = block_offset / 2;
  }

//  spinlock_unlock(&(g_free_memory.mm_spl));

  return TRUE;
}

/**
 * Set the bitmap of the offset index in the block list
 */
void set_bitmap_flag(int block_list_index, int offset, BYTE flag)
{
  BYTE* pBitmap = NULL;

  pBitmap = g_free_memory.bitmap[block_list_index].bitmap;

  if (flag == MM_TRUE) {
    // bitmap_count++ when the data is empty
    if ((pBitmap[offset / 8] & (0x01 << (offset % 8))) == 0)
      g_free_memory.bitmap[block_list_index].bitmap_count++;

    pBitmap[offset / 8] |= (0x01 << (offset % 8));
  } else {
    // bitmap_count-- when the data is exist
    if ((pBitmap[offset / 8] & (0x01 << (offset % 8))) != 0)
      g_free_memory.bitmap[block_list_index].bitmap_count--;

    pBitmap[offset / 8] &= ~(0x01 << (offset % 8));
  }
}

/**
 * Return the bitmap of the offset index in the block list
 */
BYTE get_bitmap_flag(int block_list_index, int offset)
{
  BYTE* pBitmap = 0;

  pBitmap = g_free_memory.bitmap[block_list_index].bitmap;
  if ((pBitmap[offset / 8] & (0x01 << (offset % 8))) != 0x00)
    return MM_TRUE;

  return MM_FALSE;
}

/**
 * Extend the data space by incr
 * Return start of new space allocated, or -ENOMEM for errors
 */
ssize_t sys_sbrk(ssize_t incr)
{
  ssize_t ret = 0;

  spinlock_lock(&heap_lock);
  ret = heap_end;

  if ((heap_end >= HEAP_START) && (heap_end < HEAP_START + HEAP_SIZE)) {
    heap_end += incr;

    if (PAGE_FLOOR(heap_end) > PAGE_FLOOR(ret)) {
      // Do something of VMA
    }
  } else {
    ret = -ENOMEM;
  }

  spinlock_unlock(&heap_lock);

  return ret;
}
