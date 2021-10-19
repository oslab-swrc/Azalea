// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/lock.h>
#include "console.h"
#include "map.h"
#include "memory.h"
#include "offload_page.h"

#define	shared_memory_pa(v)	(((QWORD)v)-CONFIG_SHARED_MEMORY+(g_shared_memory))

extern QWORD g_memory_start;
extern QWORD g_shared_memory;

//#define DEBUG

/**
 * @brief Get physical address corresponding to virtial address
 * @param virtual_address virtual address
 * @return success physical address, fail FALSE
 */
unsigned long get_pa(QWORD virtual_address)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  PT_ENTRY *pt_entry = NULL;
  QWORD pgd = 0;
  QWORD new_pt_page = 0;
  QWORD new_page = 0;
  int index = -1;

  // address  is NULL
  if(virtual_address == 0)
    return (0); 

  // identical mapping in high half memory
  if(virtual_address > CONFIG_HIGH_HALF_LIMIT) {
    if(virtual_address < CONFIG_PAGE_OFFSET) { // shared memory
#ifdef DEBUG
      lk_print_xy(25, 1, "get_pa(ID) va:%Q pa:%Q", virtual_address, shared_memory_pa(virtual_address));
#endif
      return (shared_memory_pa(virtual_address));
    }
    else { // kernel memory
#ifdef DEBUG
      lk_print_xy(25, 1, "get_pa(ID) va:%Q pa:%Q", virtual_address, pa(virtual_address));
#endif
      return (pa(virtual_address));
    }
  }

  spinlock_lock(&map_lock);

  //pgd = get_as(address_space_id)->pgd;
  pgd = CONFIG_KERNEL_PAGETABLE_ADDRESS;

  pml4t_entry = (PML4T_ENTRY *) va(pgd);

  index = (virtual_address & PAGE_PML4_MASK) >> PAGE_PML4_SHIFT;


  if (pml4t_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pml4t_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  // PDPT entry
  pdpt_entry = (PDPT_ENTRY *) va(pml4t_entry[index].entry & (~PAGE_ATTR_MASK));

  index = (virtual_address & PAGE_PDPT_MASK) >> PAGE_PDPT_SHIFT;
  if (pdpt_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pdpt_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  // PD entry
  pd_entry = (PD_ENTRY *) va(pdpt_entry[index].entry & (~PAGE_ATTR_MASK));

  index = (virtual_address & PAGE_PDE_MASK) >> PAGE_PDE_SHIFT;
  if (pd_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pd_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }
  else {
    // PAGE_SIZE_2M is used
    if ((pd_entry[index].entry & PAGE_FLAGS_PS) == PAGE_FLAGS_PS) {
      spinlock_unlock(&map_lock);
#ifdef DEBUG
      lk_print_xy(25, 2, "get_pa(2M) va:%Q pa:%Q", virtual_address, (pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
#endif
      return ((pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
    }
  }

  // PT entry
  pt_entry = (PT_ENTRY *) va(pd_entry[index].entry & (~PAGE_ATTR_MASK));
  index = (virtual_address & PAGE_PTE_MASK) >> PAGE_PTE_SHIFT;

  if (pt_entry[index].entry == 0) {
    new_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    //lk_print_xy(25, 4, "alloced(4K) va:%Q pa:%Q", new_page, pa(new_page));
    set_page_entry_data(&pt_entry[index], pa(new_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  spinlock_unlock(&map_lock);

#ifdef DEBUG
  lk_print_xy(25, 3, "get_pa(4K) va:%Q pa:%Q", virtual_address, (pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
  lk_print_xy(25, 4, "get_pa(4K) entry:%Q +~MASK:%Q va+MASK:%Q  ", pt_entry[index].entry, pt_entry[index].entry & (~PAGE_ATTR_MASK), virtual_address & PAGE_OFFSET_MASK);
#endif
  return ((pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
}


/**
 * @brief get_iovec for buf(with count)
 * iov->iov_base contain physical address
 * @param buf pointer to buf
 * @param count buf size
 * @param iov pointer to struct iovec which contains iov_base and iov_len
 * @param iovcnt count of iov
 * return success (0), fail (-1)
 */
int get_iovec(void *buf, size_t count, struct iovec *iov, int *iovcnt)
{
QWORD touch_buf = 0;
__attribute__((unused)) BYTE touch = 0;

QWORD page_addr = 0;		// physical address
QWORD prev_page_addr = 0;	// physical address
QWORD read_page_addr = 0;	// physical address
unsigned long read_page_len  = 0;
int offset_in_page = 0;

QWORD scan_buf = 0;			// virtual address
unsigned long scan_buf_len = 0;
unsigned long page_len = 0;

int i = 0;

  if(buf == NULL)
    return -1;
  
  // get page count of buf
  offset_in_page = ((QWORD) buf & PAGE_OFFSET_MASK);
  int nr_pages = (count + offset_in_page) / PAGE_SIZE_4K;
  (((count + offset_in_page) % PAGE_SIZE_4K) == 0) ? nr_pages : nr_pages++;

  // check page exists 
  touch_buf = (QWORD) buf - offset_in_page;
  for(i = 0; i < nr_pages; i++) {
	touch = *((BYTE *)(touch_buf + i * PAGE_SIZE_4K));
  }

  //set scan buf
  scan_buf = (QWORD) buf;
  scan_buf_len = count;

  // set initial page address
  page_addr = get_pa((QWORD) buf) - offset_in_page;
  prev_page_addr = page_addr - PAGE_SIZE_4K;
  read_page_addr = page_addr + offset_in_page;
  read_page_len = 0;

  page_len = 0;

  /* scan each page to find physically contiguous regions */
  for (i = 0; i < nr_pages; ++i) {
    /* get physcical addresses */
    offset_in_page = scan_buf & PAGE_OFFSET_MASK;
    page_addr = get_pa(scan_buf) - offset_in_page;

    /* emmit iov for a physically non-contiguous region */
    if ((unsigned long) page_addr != (unsigned long) (prev_page_addr + PAGE_SIZE_4K)) {

      iov[*iovcnt].iov_base = (void *) read_page_addr;
      iov[*iovcnt].iov_len = (size_t) read_page_len;
      (*iovcnt)++;

      /* reset region info. */
      read_page_len = 0;
      read_page_addr = get_pa(scan_buf);
    }

    /* expand this region */
    page_len = PAGE_SIZE_4K - offset_in_page; //rest of page;
    if (page_len > scan_buf_len)
      page_len = scan_buf_len;

      read_page_len += page_len;
      scan_buf += page_len;
      scan_buf_len -= page_len;
      prev_page_addr = page_addr;
    }

  iov[*iovcnt].iov_base = (void *) read_page_addr;
  iov[*iovcnt].iov_len = (size_t) read_page_len;
  (*iovcnt)++;

  return 0;
}

