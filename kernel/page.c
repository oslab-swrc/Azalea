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

/**
 * Initialize Kernel Pagetable
 */
void kernel_pagetables_init(QWORD address)
{
  PML4T_ENTRY* pml4t_entry = NULL;
  PDPT_ENTRY* pdpt_entry = NULL;
  PD_ENTRY* pd_entry = NULL;
  QWORD mapping_address = 0;
  int i = 0;

  // PML4T entry
  pml4t_entry = (PML4T_ENTRY*)va(address);

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++) // Initialize
    set_page_entry_data(&(pml4t_entry[i]), 0, 0);

  set_page_entry_data(&(pml4t_entry[0]), (address+0x1000),   PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  set_page_entry_data(&(pml4t_entry[256]), (address+0x1000), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);

  // PDPT entry
  pdpt_entry = (PDPT_ENTRY*) va(address+0x1000) ;

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry_data(&(pdpt_entry[i]), (address+0x2000)+(i*PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);

  // PD entry
  pd_entry = (PD_ENTRY*) va(address+0x2000);

  mapping_address = 0; // 0 ~ 1GB + 2MB (vcon)
  set_page_entry_data(&(pd_entry[0]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

  // 2 ~ 3 GB (shared memory)
  //mapping_address = 0;
  mapping_address = 0xA80000000;
  for (i=PAGE_MAX_ENTRY_COUNT*2; i<PAGE_MAX_ENTRY_COUNT*3; i++) {
    set_page_entry_data(&(pd_entry[i]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

    mapping_address += PAGE_SIZE_2M;
  }

  mapping_address = 0; // 3 ~ 4GB + 2MB (vcon)
  set_page_entry_data(&(pd_entry[(PAGE_MAX_ENTRY_COUNT*3)]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

  mapping_address = address ; // 3GB + 2MB ~ g_memory_end 
  for (i=PAGE_MAX_ENTRY_COUNT*3+1; i<PAGE_MAX_ENTRY_COUNT*((g_memory_end >> 30)+3); i++) {
    set_page_entry_data(&(pd_entry[i]), mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS));

    mapping_address += PAGE_SIZE_2M;
 }

  // APIC registers mapping
  mapping_address = 0xF0000000; // 3GB + 768MB ~ 4GB -> 3GB + 768MB ~ 4G (0xF000 0000)

  for (i=PAGE_MAX_ENTRY_COUNT*3+128*3; i<PAGE_MAX_ENTRY_COUNT*4; i++) {
    DWORD page_entry_flags;

    page_entry_flags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;
    set_page_entry_data(&(pd_entry[i]), mapping_address, page_entry_flags);
    mapping_address += PAGE_SIZE_2M;
  }

  mapping_address = 0x70000000; // 5GB + 768MB ~ 6GB -> 1GB + 768MB ~ 2GB

  for (i=PAGE_MAX_ENTRY_COUNT*5+128*3; i<PAGE_MAX_ENTRY_COUNT*6; i++) {
    DWORD page_entry_flags;

    page_entry_flags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;
    set_page_entry_data(&(pd_entry[i]), mapping_address, page_entry_flags);
    mapping_address += PAGE_SIZE_2M;
  }

  set_cr3(address);
}

/*
 *
 */
void adjust_pagetables(QWORD address)
{
  PML4T_ENTRY* pml4t_entry = NULL;
  PDPT_ENTRY *new_pdpt_entry = NULL;
  int i;
  
  pml4t_entry = (PML4T_ENTRY*)va(address);

  new_pdpt_entry = (PDPT_ENTRY*) alloc(PAGE_SIZE_4K); 
  
  if (new_pdpt_entry == NULL )
	{
	 debug_halt((char *) __func__, __LINE__);	
	}

  for (i=0; i<PAGE_MAX_ENTRY_COUNT; i++) {
    PD_ENTRY *new_pd_page = (PD_ENTRY *) alloc(PAGE_SIZE_4K);
    QWORD *m_addr = (QWORD *) (va((address+0x2000) + (PAGE_SIZE_4K*i)));

    lk_memcpy(new_pd_page, m_addr, PAGE_SIZE_4K);
    set_page_entry_data(&(new_pdpt_entry[i]), pa(new_pd_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  set_page_entry_data(&(pml4t_entry[0]), pa(new_pdpt_entry), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  set_page_entry_data(&(pml4t_entry[256]), pa(new_pdpt_entry), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);

/*
  // Initialize previous pages
  for (i=0; i<((g_memory_end >> 30)+3); i++) {
    QWORD *m_addr = (QWORD *) (va((address+0x1000) + (PAGE_SIZE_4K*i)));

    lk_memset((void *) m_addr, 0, PAGE_SIZE_4K);
}
*/
}

/**
 * Remove low identical mapping
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
 *
 */
void set_page_entry_data(PT_ENTRY *entry, QWORD base_address, QWORD flags)
{
  entry->entry = base_address | flags;
}
