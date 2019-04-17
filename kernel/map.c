#include <sys/lock.h>
#include "map.h"
#include "console.h"
#include "memory.h"

extern QWORD g_memory_start;

/**
 *
 */
void map_init(void)
{
  spinlock_init(&map_lock);
}
  
/**
 * Map virtial address to physical address
 */
BOOL lk_map(QWORD virtual_address, QWORD physical_address, int page_size, QWORD cache)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  PT_ENTRY *pt_entry = NULL;
  QWORD pgd = 0;
  QWORD new_pt_page = 0;
  int index = -1;
  QWORD flags = 0;

  spinlock_lock(&map_lock);

  pgd = CONFIG_KERNEL_PAGETABLE_ADDRESS;

  // PML4T entry
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

  // PT entry
  if (page_size == PAGE_SIZE_4K) {
    pt_entry = (PT_ENTRY *) va(pd_entry[index].entry & (~PAGE_ATTR_MASK));

    index = (virtual_address & PAGE_PTE_MASK) >> PAGE_PTE_SHIFT;
    flags = (cache == 0)? PAGE_FLAGS_DEFAULT: PAGE_FLAGS_NOCACHE;
    flags |= PAGE_FLAGS_US;

    set_page_entry_data(&pt_entry[index], physical_address, flags);
  } else if (page_size == PAGE_SIZE_2M) {
    flags = (cache == 0)? PAGE_FLAGS_DEFAULT: PAGE_FLAGS_NOCACHE;
    flags |= (PAGE_FLAGS_US | PAGE_FLAGS_PS);

    set_page_entry_data(&pd_entry[index], physical_address, flags);
  }

  spinlock_unlock(&map_lock);

  return TRUE;
}

/**
 * Unmap a virtual address
 *   if no more entries in the page, remove the page 
 */
BOOL lk_unmap(QWORD virtual_address)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  PT_ENTRY *pt_entry = NULL;
  QWORD pgd = 0;
  int index_pml4 = 0, index_pdpt = 0, index_pd = 0, index_pt = 0;

  spinlock_lock(&map_lock);
  pgd = CONFIG_KERNEL_PAGETABLE_ADDRESS;

  // PML4T entry
  pml4t_entry = (PML4T_ENTRY *) va(pgd);

  index_pml4 = (virtual_address & PAGE_PML4_MASK) >> PAGE_PML4_SHIFT;
  pdpt_entry = (PDPT_ENTRY *) va(pml4t_entry[index_pml4].entry & (~PAGE_ATTR_MASK));

  // PDPT entry
  index_pdpt = (virtual_address & PAGE_PDPT_MASK) >> PAGE_PDPT_SHIFT;
  pd_entry = (PD_ENTRY *) va(pdpt_entry[index_pdpt].entry & (~PAGE_ATTR_MASK));

  // PD entry
  index_pd = (virtual_address & PAGE_PDE_MASK) >> PAGE_PDE_SHIFT;

  // 2MB paging is checked by PAG_FLAGS_PS
  if (pd_entry[index_pd].entry & PAGE_FLAGS_PS) {
    set_page_entry_data(&pd_entry[index_pd], 0, 0);
  } 
  // 4KB paging
  else {
    // PT entry
    pt_entry = (PT_ENTRY *) va(pd_entry[index_pd].entry & (~PAGE_ATTR_MASK));

    index_pt = (virtual_address & PAGE_PTE_MASK) >> PAGE_PTE_SHIFT;
    set_page_entry_data(&pt_entry[index_pt], 0, 0);

    // if PT_ENTRY is empty, free it
//    if (free_page_table(pt_entry))
//      set_page_entry_data(&pd_entry[index_pd], 0, 0);
  }

  // if PD_ENTRY is empty, free it
//  if (free_page_table(pd_entry))
//    set_page_entry_data(&pdpt_entry[index_pdpt], 0, 0);

  // if PDPT_ENTRY is empty, free it
//  free_page_table(pdpt_entry);

  spinlock_unlock(&map_lock);
  return TRUE;
}

/**
 * Free page table entry when there is no used entry
 */
int free_page_table(PT_ENTRY * p)
{
  int i = 0;

  for (i = 0; i < PAGE_MAX_ENTRY_COUNT; i++)
    if (p[i].entry != 0)
      return FALSE;

  az_free((void *) va(p));

  return TRUE;
}
