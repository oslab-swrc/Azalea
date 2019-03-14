#ifndef __MPCONFIGTABLE_H__
#define __MPCONFIGTABLE_H__

#include "az_types.h"

// Feature Byte of MP Floating Pointer
#define MP_FLOATING_POINTER_FEATURE_BYTE1_USE_MP_TABLE	0x00
#define MP_FLOATING_POINTER_FEATURE_BYTE2_PIC_MODE	0x80

// Entry Type
#define MP_ENTRY_TYPE_PROCESSOR				0
#define MP_ENTRY_TYPE_BUS				1
#define MP_ENTRY_TYPE_IO_APIC				2
#define MP_ENTRY_TYPE_IO_INTERRUPT_ASSIGNMENT		3
#define MP_ENTRY_TYPE_LOCAL_INTERRUPT_ASSIGNMENT	4

// Processor CPU Flags
#define MP_PROCESSOR_CPU_FLAGS_ENABLE			0x01
#define MP_PROCESSOR_CPU_FLAGS_BSP			0x02

// Bus Type String
#define MP_BUS_TYPE_STRING_ISA				"ISA"
#define MP_BUS_TYPE_STRING_PCI				"PCI"
#define MP_BUS_TYPE_STRING_PCMCIA			"PCMCIA"
#define MP_BUS_TYPE_STRING_VESA_LOCAL_BUS		"VL"

// Interrupt Type
#define MP_INTERRUPT_TYPE_INT				0
#define MP_INTERRUPT_TYPE_NMI				1
#define MP_INTERRUPT_TYPE_SMI				2
#define MP_INTERRUPT_TYPE_EXTINT			3

// Interrupt Flags
#define MP_INTERRUPT_FLAGS_CONFORM_POLARITY		0x00
#define MP_INTERRUPT_FLAGS_ACTIVE_HIGH			0x01
#define MP_INTERRUPT_FLAGS_ACTIVE_LOW			0x03
#define MP_INTERRUPT_FLAGS_CONFORM_TRIGGER		0x00
#define MP_INTERRUPT_FLAGS_EDGE_TRIGGERED		0x04
#define MP_INTERRUPT_FLAGS_LEVEL_TRIGGERED		0x0C

// 1 Byte align
#pragma pack( push, 1 )

// MP Floating Pointer Data Structure
typedef struct mp_floating_pointer_struct {
  // Sginature, _MP_
  char signature[4];
  // MP Configuration Table address
  DWORD mp_configuration_table_address;
  // length of MP Floating Pointer Data Struct, 16 bytes
  BYTE length;
  // MultiProcessor Specification version
  BYTE revision;
  // checksum
  BYTE checksum;
  // MP Feature Byte 1~5
  BYTE mp_feature_byte[5];
} MP_FLOATING_POINTER;

// MP Configuration Table Header Data Structure
typedef struct mp_configuration_table_header_struct {
  // Signatue, PCMP
  char signature[4];
  // length of Basic table
  WORD base_table_length;
  // MultiProcessor Specification verson
  BYTE revision;
  // chucksum
  BYTE checksum;
  // OEM ID 
  char oem_id_string[8];
  // PRODUCT ID
  char product_id_string[12];
  // OEM Table Pointer address
  DWORD oem_table_pointer_address;
  // size of OEM table
  WORD oem_table_size;
  // count of MP basic table entry
  WORD entry_count;
  // Map IO address of local APIC 
  DWORD memory_map_io_address_of_local_apic;
  // length of Extened table
  WORD extended_table_length;
  // chucksum of Extened table
  BYTE extended_table_checksum;
  // reserved
  BYTE reserved;
} MP_CONFIGURATION_TABLE_HEADER;

// Processor Entry
typedef struct processor_entry_struct {
  // Entry type code, 0
  BYTE entry_type;
  // local APIC id 
  BYTE local_apic_id;
  // local APIC version
  BYTE local_apic_version;
  // CPU Flags
  BYTE cpu_flags;
  // CPU Signatue
  BYTE cpu_signature[4];
  // Feature Flags
  DWORD feature_flags;
  // reseved
  DWORD reserved[2];
} PROCESSOR_ENTRY;

// Bus Entry
typedef struct bus_entry_struct {
  BYTE entry_type;
  BYTE bus_id;
  char bus_type_string[6];
} BUS_ENTRY;

// I/O APIC Entry
typedef struct io_apic_entry_struct {
  BYTE entry_type;
  BYTE io_apic_id;
  BYTE io_apic_version;
  BYTE io_apic_flags;
  DWORD memory_map_address;
} IO_APIC_ENTRY;

// I/O Interrupt Assignment Entry
typedef struct io_interrupt_assignment_entry_struct {
  // Entry type code, 3
  BYTE entry_type;
  BYTE interrupt_type;
  WORD interrupt_flags;
  BYTE source_bus_id;
  BYTE source_bus_irq;
  BYTE destination_io_apic_id;
  BYTE destination_io_apic_lint_in;
} IO_INTERRUPT_ASSIGNMENT_ENTRY;

// Local Interrupt Assignment Entry
typedef struct local_interrupt_entry_struct {
  // Entry type code, 4
  BYTE entry_type;
  BYTE interrupt_type;
  WORD interrupt_flags;
  BYTE source_bus_id;
  BYTE source_bus_irq;
  BYTE destination_local_apic_id;
  BYTE destination_local_apic_lint_in;
} LOCAL_INTERRUPT_ASSIGNMENT_ENTRY;

#pragma pack( pop)

// Manager Data Structure of MP Configuration Table
typedef struct mp_configuration_manager_struct {
  MP_FLOATING_POINTER *mp_floating_pointer;
  MP_CONFIGURATION_TABLE_HEADER *mp_configuration_table_header;
  QWORD base_entry_start_address;
  int processor_count;
  BOOL use_pic_mode;
  BYTE isa_bus_id;
} MP_CONFIG_MANAGER;

BOOL find_mp_floating_pointer_address(QWORD * address);
BOOL analysis_mp_config_table(void);
MP_CONFIG_MANAGER *get_mp_config_manager(void);
int get_processor_count(void);
IO_APIC_ENTRY *find_io_apic_entry_for_isa(void);

#endif                          // __MPCONFIGTABLE_H__
