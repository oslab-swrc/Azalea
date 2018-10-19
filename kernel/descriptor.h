#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "memory_config.h"
#include "multiprocessor.h"
#include "types.h"

/* #define MAX_PROCESSOR_COUNT	1 */

#define GDT_TYPE_CODE		0x0A
#define GDT_TYPE_DATA		0x02
#define GDT_TYPE_TSS		0x09
#define GDT_FLAGS_LOWER_S	0x10
#define GDT_FLAGS_LOWER_DPL0	0x00
#define GDT_FLAGS_LOWER_DPL1	0x20
#define GDT_FLAGS_LOWER_DPL2	0x40
#define GDT_FLAGS_LOWER_DPL3	0x60
#define GDT_FLAGS_LOWER_P	0x80
#define GDT_FLAGS_UPPER_L	0x20
#define GDT_FLAGS_UPPER_DB	0x40
#define GDT_FLAGS_UPPER_G	0x80

#define GDT_FLAGS_LOWER_KERNEL_CODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNEL_DATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USER_CODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USER_DATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS (GDT_FLAGS_UPPER_G)

// Segment descriptor offset
#define GDT_KERNEL_CODE_SEGMENT	0x08
#define GDT_KERNEL_DATA_SEGMENT	0x10
#define GDT_USER_DATA_SEGMENT	0x18
#define GDT_USER_CODE_SEGMENT	0x20
#define GDT_TSS			0x28

#define GDTR_START_ADDRESS	(0x242000+g_memory_start)

#define GDT_MAX_ENTRY8_COUNT	5
#define GDT_MAX_ENTRY16_COUNT	(MAX_PROCESSOR_COUNT)
#define GDT_TABLE_SIZE ((sizeof(GDT_ENTRY8)*GDT_MAX_ENTRY8_COUNT)+\
		(sizeof(GDT_ENTRY16)*GDT_MAX_ENTRY16_COUNT))
#define TSS_SEGMENT_SIZE	(sizeof(TSS)*MAX_PROCESSOR_COUNT)

#define SELECTOR_RPL_0		0x00
#define SELECTOR_RPL_3		0x03


// IDT
#define IDT_TYPE_INTERRUPT	0x0E
#define IDT_TYPE_TRAP		0x0F
#define IDT_FLAGS_DPL0		0x00
#define IDT_FLAGS_DPL1		0x20
#define IDT_FLAGS_DPL2		0x40
#define IDT_FLAGS_DPL3		0x60
#define IDT_FLAGS_P		0x80
#define IDT_FLAGS_IST0		0
#define IDT_FLAGS_IST1		1

#define IDT_FLAGS_KERNEL	(IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER		(IDT_FLAGS_DPL3 | IDT_FLAGS_P)

#define IDT_MAX_ENTRY_COUNT	100
#define IDTR_START_ADDRESS	(GDTR_START_ADDRESS + sizeof(GDTR) + GDT_TABLE_SIZE + TSS_SEGMENT_SIZE)
#define IDT_START_ADDRESS	(IDTR_START_ADDRESS + sizeof(IDTR))
#define IDT_TABLE_SIZE		(IDT_MAX_ENTRY_COUNT*sizeof(IDT_ENTRY))

#define IST_START_ADDRESS	(0xA00000+g_memory_start)
#define IST_SIZE		0x100000

#pragma pack(push, 1)

// GDTR
typedef struct gdtr_struct {
  WORD limit;
  QWORD base_address;
  WORD pading16;
  DWORD pading32;
} GDTR, IDTR;

// GDTR entry8
typedef struct gdt_entry8_struct {
  WORD lower_limit;
  WORD lower_base_address;
  BYTE upper_base_address1;
  BYTE type_and_lower_flag;
  BYTE upper_limit_and_upper_flag;
  BYTE upper_base_address2;
} GDT_ENTRY8;

// GDTR entry16
typedef struct gdt_entry16_struct {
  WORD lower_limit;
  WORD lower_base_address;
  BYTE middle_base_address1;
  BYTE type_and_lower_flag;
  BYTE upper_limit_and_upper_flag;
  BYTE middle_base_address2;
  DWORD upper_base_address;
  DWORD reserved;
} GDT_ENTRY16;

// TSS
typedef struct tss_data_struct {
  DWORD reserved1;
  QWORD rsp[3];
  QWORD reserved2;
  QWORD ist[7];
  QWORD reserved3;
  WORD reserved;
  WORD io_map_base_address;
} TSS;

// IDT entry
typedef struct idt_entry_struct {
  WORD lower_base_address;
  WORD segment_selector;
  BYTE ist;
  BYTE type_and_flags;
  WORD middle_base_address;
  DWORD upper_base_address;
  DWORD reserved;
} IDT_ENTRY;

#pragma pack(pop)

void dummy_handler(void);
void gdt_and_tss_init(void);
void idt_init(void);
void initialize_tss(TSS * tss);
void set_gdt_entry8(GDT_ENTRY8 * entry, DWORD base_address,
                   DWORD limit, BYTE upper_flags, BYTE lower_flags,
                   BYTE type);
void set_gdt_entry16(GDT_ENTRY16 * entry, QWORD base_address,
                    DWORD limit, BYTE upper_flags, BYTE lower_flags,
                    BYTE type);
void set_idt_entry(IDT_ENTRY * entry, void *handler, WORD selector,
                  BYTE ist, BYTE flags, BYTE type);
#endif                          // __DESCRIPTOR_H__
