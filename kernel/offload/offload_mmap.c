#include <sys/lock.h>
#include "console.h"
#include "map.h"
#include "memory.h"

#include "offload_mmap.h"

extern QWORD g_memory_start;

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

  // identical mapping in high half memory
  if(virtual_address > CONFIG_HIGH_HALF_LIMIT) {
#if 0
  lk_print_xy(25, 1, "get_pa(ID) va:%Q pa:%Q", virtual_address, pa(virtual_address));
#endif
    return (pa(virtual_address)); 
  }

  spinlock_lock(&map_lock);

  //pgd = get_as(address_space_id)->pgd;
  pgd = CONFIG_KERNEL_PAGETABLE_ADDRESS;

  pml4t_entry = (PML4T_ENTRY *) va(pgd);

  index = (virtual_address & PAGE_PML4_MASK) >> PAGE_PML4_SHIFT;


  if (pml4t_entry[index].entry == 0) {
    new_pt_page = (QWORD) alloc(PAGE_SIZE_4K);

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
    new_pt_page = (QWORD) alloc(PAGE_SIZE_4K);

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
    new_pt_page = (QWORD) alloc(PAGE_SIZE_4K);

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
#if 0
      lk_print_xy(25, 2, "get_pa(2M) va:%Q pa:%Q", virtual_address, (pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
#endif
      return ((pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
    }
  }

  // PT entry
  pt_entry = (PT_ENTRY *) va(pd_entry[index].entry & (~PAGE_ATTR_MASK));
  index = (virtual_address & PAGE_PTE_MASK) >> PAGE_PTE_SHIFT;

  if (pt_entry[index].entry == 0) {
    new_page = (QWORD) alloc(PAGE_SIZE_4K);

    if (new_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    //lk_print_xy(25, 4, "alloced(4K) va:%Q pa:%Q", new_page, pa(new_page));
    set_page_entry_data(&pt_entry[index], pa(new_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  spinlock_unlock(&map_lock);

#if 0
  lk_print_xy(25, 3, "get_pa(4K) va:%Q pa:%Q", virtual_address, (pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
#endif
  return ((pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
}


