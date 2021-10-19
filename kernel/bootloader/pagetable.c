// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../include/qemu/arch.h"
#include "page_32.h"

/*
 * set page entry
 */
void set_page_entry(PT_ENTRY * entry, DWORD upper_base_address,
                    DWORD lower_base_address, DWORD lower_flags,
                    DWORD upper_flags)
{
  entry->attribute_and_lower_base_address =
      lower_base_address | lower_flags;
  entry->upper_base_address_and_exb =
      (upper_base_address & 0xFF) | upper_flags;
}

/*
 * initialize page table
 */
static void init_page_table(QWORD base_va)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  QWORD mapping_address = 0, upper_address = 0;
  int i = 0;
  QWORD base_pa = BOOT_ADDR + PAGE_4K ;
  
  // initialize PML4T_ENTRY
  pml4t_entry = (PML4T_ENTRY *) base_va;

  for (i = 0; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pml4t_entry[i]), 0, 0, 0, 0);

  set_page_entry(&(pml4t_entry[0]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pml4t_entry[256]), 0x00, (base_pa + PAGE_4K),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  // initialize PDPT_ENTRY
  pdpt_entry = (PDPT_ENTRY *) (base_va + PAGE_4K);

  set_page_entry(&(pdpt_entry[0]), 0, (base_pa + 0x2000) + (0 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  set_page_entry(&(pdpt_entry[1]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[2]), 0, 0, 0, 0);
  set_page_entry(&(pdpt_entry[3]), 0, (base_pa + 0x2000) + (1 * PAGE_TABLE_SIZE),
                 PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

  for (i = 4; i < PAGE_MAX_ENTRY_COUNT; i++)
    set_page_entry(&(pdpt_entry[i]), 0, 0, 0, 0);

  // initialize PD_ENTRY
  pd_entry = (PD_ENTRY *) (base_va + 0x2000);

  mapping_address = 0;
  set_page_entry(&(pd_entry[0]), 0, mapping_address,
                 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

  mapping_address = ((QWORD) MEMORY_START + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) MEMORY_START + 0x200000) >> 32;
  for (i = 1; i < PAGE_MAX_ENTRY_COUNT; i++) {
    set_page_entry(&(pd_entry[i]), upper_address, mapping_address, 
		 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

    mapping_address += PAGE_DEFAULT_SIZE;
  }

/*
  virtual -> physical (256MB*3 = 768MB)
  0xC000_0000 -> 0x0000_0000
  ...
  0xEFFF_FFFF -> 0x2FFF_FFFF
*/

  mapping_address = 0;
  set_page_entry(&(pd_entry[(PAGE_MAX_ENTRY_COUNT)]),
                 (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12, mapping_address,
                 (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS), 0);

  mapping_address = ((QWORD) MEMORY_START + 0x200000) & 0xFFFFFFFF;
  upper_address = ((QWORD) MEMORY_START + 0x200000) >> 32;

  for (i = PAGE_MAX_ENTRY_COUNT + 1; i < PAGE_MAX_ENTRY_COUNT + 128 * 3; i++) {
    set_page_entry(&(pd_entry[i]), upper_address,
                   mapping_address, (PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS),
                   0);

    mapping_address += PAGE_DEFAULT_SIZE;
  }
/*
  It's for accessing APIC registers.

  virtual -> physical (256MB)
  0xF000_0000 -> 0xF000_0000
  ...
  0xFFFF_FFFF -> 0xFFFF_FFFF
*/
  mapping_address = 0xF0000000;
  for (; i < PAGE_MAX_ENTRY_COUNT * 2; i++) {
    DWORD dwPageEntryFlags;

    dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;

    set_page_entry(&(pd_entry[i]), (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12,
                   mapping_address, dwPageEntryFlags, 0);
    mapping_address += PAGE_DEFAULT_SIZE;
  }
}

void main()
{
	
	char pagetable[4096*4] ;
	
	init_page_table((QWORD)pagetable) ;
	
	int fd = 0 ;

	fd = open("page.bin", O_CREAT | O_WRONLY, 777) ;

	write(fd, pagetable, 4096*4) ;

	close(fd) ;
 
}

