#include "arch.h"
#include "page.h"
#include "assemblyutility.h"
#include "console.h"
#include "debug.h"
#include "memory.h"
#include "multiprocessor.h"
#include "thread.h"
#include "utility.h"

extern QWORD g_memory_start;
extern QWORD g_memory_end;
extern QWORD g_shared_memory;

/**
 * @brief Initialize Kernel Pagetable
 * @brief Requrired 6 pages (PML4T(1), PDPT(1), PD(2))
 * @brief Maxinum memory is 512GB (depends on the PDPT)
 * @param address - pagetable address
 * @return none
 */
void kernel_pagetables_init(QWORD address)
{
  PML4T_ENTRY* pml4t_entry = NULL;
  PDPT_ENTRY* pdpt_entry = NULL;
  PD_ENTRY* pd_entry = NULL;
  QWORD mapping_address = 0;
  int i = 0;

  // PML4T entry
  pml4t_entry = (PML4T_ENTRY*) va(address);

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++) // initialize
    set_page_entry_data(&(pml4t_entry[i]), 0, 0);

  set_page_entry_data(&(pml4t_entry[0]), (address+0x1000),   PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  set_page_entry_data(&(pml4t_entry[256]), (address+0x1000), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);

  // PDPT entry (0, 3th entry)
  pdpt_entry = (PDPT_ENTRY*) va(address+0x1000) ;

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++) // initialize
    set_page_entry_data(&(pdpt_entry[i]), 0, 0);

  set_page_entry_data(&(pdpt_entry[0]), address+0x2000, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US); 
  set_page_entry_data(&(pdpt_entry[3]), address+0x3000, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US); 

  // PD entry (0)
  pd_entry = (PD_ENTRY*) va(address+0x2000); 

  set_page_entry_data(&(pd_entry[0]), 0, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

  // PDPT entry (2nd~3th, shared memory)
  mapping_address = g_shared_memory;
  for (i=1; i<3; i++) { 
    set_page_entry_data(&(pdpt_entry[i]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US)); 
    mapping_address += PAGE_SIZE_1G;
  }

  // PD entry (3th, kernel)
  pd_entry = (PD_ENTRY*) va(address+0x3000); 

  //   3GB ~ 3GB + 2MB 
  set_page_entry_data(&(pd_entry[0]), 0, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS)); 

  //   3GB + 2MB ~ 4GB
  mapping_address = address ;
  for (i=1; i<PAGE_MAX_ENTRY_COUNT; i++) {
    set_page_entry_data(&(pd_entry[i]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

    mapping_address += PAGE_SIZE_2M;
  }

  // PDPT entry (4th~, free memory)
  for (i=4; i<((g_memory_end-g_memory_start)>>30)+4; i++) {
    set_page_entry_data(&(pdpt_entry[i]), mapping_address, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US);

    mapping_address += PAGE_SIZE_1G;
  }

  set_cr3(address);
}

/**
 * @brief Remove low identical mapping
 * @param address - pagetable address
 * return none
 */
void remove_low_identical_mapping(QWORD pgd) 
{
  PML4T_ENTRY* pml4_entry = NULL;
  PD_ENTRY *pd_entry = (PD_ENTRY *)va(pgd+0x2000);
  int i = 0;

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++) {
    pd_entry[i].entry = 0;
  }

  pml4_entry = (PML4T_ENTRY *) va(pgd);
  pml4_entry[0].entry = 0;
}

/**
 * @brief Set pagetable entry with flags
 * @param entry - pointer of the target pagetable entry
 * @param base_address - base address for the entry
 * @param flags - flags for the pagetable entry
 * return none
 */
void set_page_entry_data(PT_ENTRY *entry, QWORD base_address, QWORD flags)
{
  entry->entry = base_address | flags;
}
