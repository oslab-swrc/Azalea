// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __PAGE_H__
#define __PAGE_H__

#include "memory_config.h"
#include "az_types.h"

#define PAGE_FLAGS_P		0x00000001	// Present
#define PAGE_FLAGS_RW		0x00000002	// R/W
#define PAGE_FLAGS_US		0x00000004	// U/S
#define PAGE_FLAGS_PWT		0x00000008	// Page level write-through
#define PAGE_FLAGS_PCD		0x00000010	// Page level cache disable
#define PAGE_FLAGS_A		0x00000020	// Accessed
#define PAGE_FLAGS_D		0x00000040	// Dirty
#define PAGE_FLAGS_PS		0x00000080	// Page size - 0:4KB, 1:2M
#define PAGE_FLAGS_G		0x00000100	// Global
#define PAGE_FLAGS_PAT		0x00001000	// Page attribute table index
#define PAGE_FLAGS_EXB		0x80000000	// Execute disable bit
#define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW)
#define PAGE_FLAGS_NOCACHE	(PAGE_FLAGS_P | PAGE_FLAGS_RW | PAGE_FLAGS_PCD)
//#define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW | PAGE_FLAGS_PCD | PAGE_FLAGS_PWT)

#define PAGE_PML4_MASK		(0xff8000000000)
#define PAGE_PDPT_MASK		(0x007fc0000000)
#define PAGE_PDE_MASK		(0x00003fe00000)
#define PAGE_PTE_MASK		(0x0000001ff000)
#define PAGE_OFFSET_MASK	(0x000000000fff)

#define PAGE_PML4_SHIFT		(39)
#define PAGE_PDPT_SHIFT		(30)
#define PAGE_PDE_SHIFT		(21)
#define PAGE_PTE_SHIFT		(12)

#define PAGE_ATTR_MASK		(0xFFF)

#define PAGE_TABLESIZE		0x1000
#define PAGE_MAX_ENTRY_COUNT	512

#define PAGE_DEFAULT_SIZE   0x1000
#define PAGE_SIZE_4K        0x1000
#define PAGE_SIZE_2M        0x200000
#define PAGE_SIZE_1G        0x40000000

#define PAGE_OFFSET		(CONFIG_PAGE_OFFSET)

#define __va(p) (((QWORD)p)+PAGE_OFFSET)
#define __pa(v) (((QWORD)v)-PAGE_OFFSET)

#define va(p)	(((QWORD)p)+PAGE_OFFSET-(g_memory_start))
#define pa(v)	(((QWORD)v)-PAGE_OFFSET+(g_memory_start))

#pragma pack(push, 1)

typedef struct page_table_entry_struct {
	QWORD entry;
} PML4T_ENTRY, PDPT_ENTRY, PD_ENTRY, PT_ENTRY;

#pragma pack(pop)

// Functions

void kernel_pagetables_init(QWORD address);
void remove_low_identical_mapping(QWORD pgd);
void set_page_entry_data(PT_ENTRY *entry, QWORD base_address, QWORD flags);

#endif /* __PAGE_H__ */

