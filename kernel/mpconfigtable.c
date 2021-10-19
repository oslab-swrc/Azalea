// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "console.h"
#include "mpconfigtable.h"
#include "page.h"
#include "utility.h"

static MP_CONFIGURATION_TABLE_HEADER g_mp_header = {{0, }, };

static MP_CONFIG_MANAGER g_mp_config_manager = {0, };

extern QWORD g_memory_start;

/*
 * find address of MP floating pointer
 */
BOOL find_mp_floating_pointer_address(QWORD * address)
{
  char *mp_floating_pointer = NULL;
  QWORD ebda_address = 0;
  QWORD system_base_memory = 0;

  printk("Extended BIOS Data Area = [0x%X] ", (DWORD) (*(WORD *) 0x040E) * 16);
  printk("System Base Address = [0x%X] ", (DWORD) (*(WORD *) 0x0413) * 1024);

  ebda_address = *(WORD *) (0x040E);
  ebda_address *= 16;

  for (mp_floating_pointer = (char *) ebda_address;
       (QWORD) mp_floating_pointer <= (ebda_address + 1024);
       mp_floating_pointer++) {
    if (lk_memcmp(mp_floating_pointer, "_MP_", 4) == 0) {
      printk("MP Floating Pinter is in EBDA, [0x%X] Address", (QWORD) mp_floating_pointer);
      *address = (QWORD) mp_floating_pointer;
      return TRUE;
    }
  }

  system_base_memory = *(WORD *) 0x0413;
  system_base_memory *= 1024;

  for (mp_floating_pointer = (char *) (system_base_memory - 1024);
       (QWORD) mp_floating_pointer <= system_base_memory;
       mp_floating_pointer++) {

    if (lk_memcmp(mp_floating_pointer, "_MP_", 4) == 0) {
      printk("MP Floating Pointer is in system base memory, [0x%X] address", (QWORD) mp_floating_pointer);
      *address = (QWORD) mp_floating_pointer;
      return TRUE;
    }
  }

  for (mp_floating_pointer = (char *) 0x0F0000;
       (QWORD) mp_floating_pointer < 0x0FFFFF; mp_floating_pointer++) {
    if (lk_memcmp(mp_floating_pointer, "_MP_", 4) == 0) {
      printk("MP Floating Pointer is in ROM, [0x%X] address",
             (QWORD) mp_floating_pointer);
      *address = (QWORD) mp_floating_pointer;
      return TRUE;
    }
  }

  return FALSE;
}

/*
 * analyze MP config table
 */
BOOL analysis_mp_config_table(void)
{
#if 1
  // initialize data structure
  lk_memset(&g_mp_config_manager, 0, sizeof(MP_CONFIG_MANAGER));
  g_mp_config_manager.isa_bus_id = 0xFF;

  g_mp_header.memory_map_io_address_of_local_apic = *(unsigned int *) (CONFIG_APIC_ADDR);
  g_mp_config_manager.mp_configuration_table_header = (MP_CONFIGURATION_TABLE_HEADER *) pa(&g_mp_header);

  return TRUE;
#else
  QWORD qwMPFloatingPointerAddress;
  MP_FLOATING_POINTER *pstMPFloatingPointer;
  MP_CONFIGURATION_TABLE_HEADER *pstMPConfigurationHeader;
  BYTE bEntryType;
  WORD i;
  QWORD qwEntryAddress;
  PROCESSOR_ENTRY *pstProcessorEntry;
  BUS_ENTRY *pstBusEntry;

  // 자료구조 초기화
  lk_memset(&g_mp_config_manager, 0, sizeof(MP_CONFIG_MANAGER));
  g_mp_config_manager.isa_bus_id = 0xFF;

  // MP 플로팅 포인터의 어드레스를 구함
  if (find_mp_floating_pointer_address(&qwMPFloatingPointerAddress) == FALSE) {
    return FALSE;
  }
  // MP 플로팅 테이블 설정
  pstMPFloatingPointer = (MP_FLOATING_POINTER *) qwMPFloatingPointerAddress;
  g_mp_config_manager.mp_floating_pointer = pstMPFloatingPointer;
  pstMPConfigurationHeader = (MP_CONFIGURATION_TABLE_HEADER *)
      ((QWORD) pstMPFloatingPointer->mp_configuration_table_address &
       0xFFFFFFFF);

  // PIC 모드 지원 여부 저장
  if (pstMPFloatingPointer->mp_feature_byte[1] &
      MP_FLOATING_POINTER_FEATURE_BYTE2_PIC_MODE) {
    g_mp_config_manager.use_pic_mode = TRUE;
  }
  // MP 설정 테이블 헤더와 기본 MP 설정 테이블 엔트리의 시작 어드레스 설정
  g_mp_config_manager.mp_configuration_table_header =
      pstMPConfigurationHeader;
  g_mp_config_manager.base_entry_start_address =
      pstMPFloatingPointer->mp_configuration_table_address +
      sizeof(MP_CONFIGURATION_TABLE_HEADER);
  // 모든 엔트리를 돌면서 프로세서의 코어 수를 계산하고 ISA 버스를 검색하여 ID를 저장
  qwEntryAddress = g_mp_config_manager.base_entry_start_address;
  for (i = 0; i < pstMPConfigurationHeader->entry_count; i++) {
    bEntryType = *(BYTE *) qwEntryAddress;
    switch (bEntryType) {
     // 프로세서 엔트리이면 프로세서의 수를 하나 증가시킴
    case MP_ENTRY_TYPE_PROCESSOR:
      pstProcessorEntry = (PROCESSOR_ENTRY *) qwEntryAddress;
      if (pstProcessorEntry->cpu_flags & MP_PROCESSOR_CPU_FLAGS_ENABLE) {
        g_mp_config_manager.processor_count++;
      }
      qwEntryAddress += sizeof(PROCESSOR_ENTRY);
      break;

     // 버스 엔트리이면 ISA 버스인지 확인하여 저장
    case MP_ENTRY_TYPE_BUS:
      pstBusEntry = (BUS_ENTRY *) qwEntryAddress;
      if (lk_memcmp(pstBusEntry->bus_type_string, MP_BUS_TYPE_STRING_ISA,
                  lk_strlen(MP_BUS_TYPE_STRING_ISA)) == 0) {
        g_mp_config_manager.isa_bus_id = pstBusEntry->bus_id;
      }
      qwEntryAddress += sizeof(BUS_ENTRY);
      break;

    // 기타 엔트리는 무시하고 이동
    case MP_ENTRY_TYPE_IO_APIC:
    case MP_ENTRY_TYPE_IO_INTERRUPT_ASSIGNMENT:
    case MP_ENTRY_TYPE_LOCAL_INTERRUPT_ASSIGNMENT:
    default:
      qwEntryAddress += 8;
      break;
    }
  }
  //g_mp_config_manager.bUsePICMode = 1; // shson
  /* printk("iProcessorCount=%d\n", g_mp_config_manager.iProcessorCount); */
  /* while (1); */

  return TRUE;
#endif
}

/*
 * get MP Config Manager
 */
MP_CONFIG_MANAGER *get_mp_config_manager(void)
{
  return &g_mp_config_manager;
}


/*
 * get Processor Count
 */
int get_processor_count(void)
{
  if (g_mp_config_manager.processor_count == 0)
    return 1;
  return g_mp_config_manager.processor_count;
}


/*
 * find IO APIC entry for ISA
 */
IO_APIC_ENTRY *find_io_apic_entry_for_isa(void)
{
  MP_CONFIGURATION_TABLE_HEADER *mp_header = NULL;
  IO_INTERRUPT_ASSIGNMENT_ENTRY *io_assignment_entry = NULL;
  IO_APIC_ENTRY *io_apic_entry = NULL;
  QWORD entry_address = 0;
  BYTE entry_type = 0;
  BOOL find = FALSE;
  int i = 0;

  mp_header = g_mp_config_manager.mp_configuration_table_header;
  entry_address = g_mp_config_manager.base_entry_start_address;

  for (i = 0; (i < mp_header->entry_count) && (find == FALSE); i++) {
    entry_type = *(BYTE *) entry_address;
    switch (entry_type) {
    case MP_ENTRY_TYPE_PROCESSOR:
      entry_address += sizeof(PROCESSOR_ENTRY);
      break;
    case MP_ENTRY_TYPE_BUS:
    case MP_ENTRY_TYPE_IO_APIC:
    case MP_ENTRY_TYPE_LOCAL_INTERRUPT_ASSIGNMENT:
      entry_address += 8;
      break;
    case MP_ENTRY_TYPE_IO_INTERRUPT_ASSIGNMENT:
      io_assignment_entry = (IO_INTERRUPT_ASSIGNMENT_ENTRY *) entry_address;
      if (io_assignment_entry->source_bus_id ==
          g_mp_config_manager.isa_bus_id)
        find = TRUE;
      entry_address += sizeof(IO_INTERRUPT_ASSIGNMENT_ENTRY);
      break;
    default:
      break;
    }
  }

  if (find == FALSE)
    return NULL;

  entry_address = g_mp_config_manager.base_entry_start_address;
  for (i = 0; i < mp_header->entry_count; i++) {
    entry_type = *(BYTE *) entry_address;
    switch (entry_type) {
    case MP_ENTRY_TYPE_PROCESSOR:
      entry_address += sizeof(PROCESSOR_ENTRY);
      break;
    case MP_ENTRY_TYPE_BUS:
    case MP_ENTRY_TYPE_IO_INTERRUPT_ASSIGNMENT:
    case MP_ENTRY_TYPE_LOCAL_INTERRUPT_ASSIGNMENT:
      entry_address += 8;
      break;
    case MP_ENTRY_TYPE_IO_APIC:
      io_apic_entry = (IO_APIC_ENTRY *) entry_address;
      if (io_apic_entry->io_apic_id ==
          io_assignment_entry->destination_io_apic_id)
        return io_apic_entry;
      entry_address += sizeof(IO_INTERRUPT_ASSIGNMENT_ENTRY);
      break;
    default:
      break;
    }
  }

  return NULL;
}
