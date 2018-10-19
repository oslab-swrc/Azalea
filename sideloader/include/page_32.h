#ifndef __PAGE_H__
#define __PAGE_H__

#define DWORD	unsigned int 
#define QWORD	unsigned long

#define PAGE_FLAGS_P	0x0000000000000001	// Present
#define PAGE_FLAGS_RW	0x0000000000000002	// R/W
#define PAGE_FLAGS_US	0x0000000000000004	// U/S
#define PAGE_FLAGS_PWT	0x0000000000000008	// Page level write-through
#define PAGE_FLAGS_PCD	0x0000000000000010	// Page level cache disable
#define PAGE_FLAGS_A	0x0000000000000020	// Accessed
#define PAGE_FLAGS_D	0x0000000000000040	// Dirty
#define PAGE_FLAGS_PS	0x0000000000000080	// Page size
#define PAGE_FLAGS_G	0x0000000000000100	// Global
#define PAGE_FLAGS_PAT	0x0000000000001000	// Page attribute table index
#define PAGE_FLAGS_EXB	0x8000000000000000	// Execute disable bit
// #define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW)
#define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW | PAGE_FLAGS_PCD)

#define PAGE_TABLESIZE	0x1000
#define PAGE_MAXENTRYCOUNT	512
#define PAGE_DEFAULTSIZE	0x200000

#define DYNAMICMEMORY_START_ADDRESS	(10 << 20)	// 10MB

#pragma pack(push, 1)
typedef struct kPageTableEntryStruct {
	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;
#pragma pack(pop)

unsigned long long kInitializePageTables(DWORD address);
void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags);

#endif // __PAGE_H__

