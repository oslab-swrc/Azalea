#include "console.h"
#include "descriptor.h"
#include "isr.h"
#include "page.h"
#include "utility.h"

extern QWORD g_memory_start;

TSS *g_tss = NULL;

/**
 * initialize GDT and TSS
 */
void gdt_and_tss_init(void)
{
  GDTR *gdtr = NULL;
  GDT_ENTRY8 *entry = NULL;
  TSS *g_tss = NULL;
  int i = 0;

  gdtr = (GDTR *) va(GDTR_START_ADDRESS);
  entry = (GDT_ENTRY8 *) (va(GDTR_START_ADDRESS) + sizeof(GDTR));
  gdtr->limit = GDT_TABLE_SIZE - 1;
  gdtr->base_address = (QWORD) entry;

  g_tss = (TSS *) ((QWORD) entry + GDT_TABLE_SIZE);

  set_gdt_entry8(((GDT_ENTRY8 *) & entry[0]), 0, 0, 0, 0, 0);

  set_gdt_entry8(((GDT_ENTRY8 *) & entry[1]), 0, 0xFFFF,
                GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNEL_CODE,
                GDT_TYPE_CODE);

  set_gdt_entry8(((GDT_ENTRY8 *) & entry[2]), 0, 0xFFFF,
                GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNEL_DATA,
                GDT_TYPE_DATA);

  set_gdt_entry8(((GDT_ENTRY8 *) & entry[3]), 0, 0xFFFF,
                GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_USER_DATA,
                GDT_TYPE_DATA);

  set_gdt_entry8(((GDT_ENTRY8 *) & entry[4]), 0, 0xFFFF,
                GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_USER_CODE,
                GDT_TYPE_CODE);

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    set_gdt_entry16(((GDT_ENTRY16 *) & entry[GDT_MAX_ENTRY8_COUNT + i * 2]),
                   (QWORD) g_tss + (i * sizeof(TSS)), sizeof(TSS) - 1,
                   GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);
  }

  initialize_tss(g_tss);
}

/*
 * set GDT Entry 8
 */
void set_gdt_entry8(GDT_ENTRY8 * entry, DWORD base_address,
                   DWORD limit, BYTE upper_flags, BYTE lower_flags,
                   BYTE type)
{
  entry->lower_limit = limit & 0xFFFF;
  entry->lower_base_address = base_address & 0xFFFF;
  entry->upper_base_address1 = (base_address >> 16) & 0xFF;
  entry->type_and_lower_flag = lower_flags | type;
  entry->upper_limit_and_upper_flag =
      ((limit >> 16) & 0xFF) | upper_flags;
  entry->upper_base_address2 = (base_address >> 24) & 0xFF;
}

/*
 * set GDT Entry 16
 */
void set_gdt_entry16(GDT_ENTRY16 * entry, QWORD base_address,
                    DWORD limit, BYTE upper_flags, BYTE lower_flags,
                    BYTE type)
{
  entry->lower_limit = limit & 0xFFFF;
  entry->lower_base_address = base_address & 0xFFFF;
  entry->middle_base_address1 = (base_address >> 16) & 0xFF;
  entry->type_and_lower_flag = lower_flags | type;
  entry->upper_limit_and_upper_flag =
      ((limit >> 16) & 0xFF) | upper_flags;
  entry->middle_base_address2 = (base_address >> 24) & 0xFF;
  entry->upper_base_address = base_address >> 32;
  entry->reserved = 0;
}

/*
 * initialize TSS
 */
void initialize_tss(TSS * tss)
{
  int i = 0;

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    // Initialize to 0
    lk_memset(tss, 0, sizeof(TSS));
    // Allocate IST from the end of the IST_ADDRESS
    tss->ist[0] = va(IST_START_ADDRESS + IST_SIZE - (IST_SIZE / MAX_PROCESSOR_COUNT * i));
    // Do not use IO Map
    // By setting the base address of the IO Map to be larger than the limit field of the TSS descriptor
    tss->io_map_base_address = 0xFFFF;

    tss++;
  }
}

/*
 * set IDT Entry
 */
void set_idt_entry(IDT_ENTRY * entry, void *handler, WORD selector,
                  BYTE ist, BYTE flags, BYTE type)
{
  entry->lower_base_address = (QWORD) handler & 0xFFFF;
  entry->segment_selector = selector;
  entry->ist = ist & 0x3;
  entry->type_and_flags = type | flags;
  entry->middle_base_address = ((QWORD) handler >> 16) & 0xFFFF;
  entry->upper_base_address = (QWORD) handler >> 32;
  entry->reserved = 0;
}

void dummy_handler(void)
{
  prints_xy(0, 0, "Dummy interrupt handler");

  while (1);
}

/* 
 * initialize IDT
 */
void idt_init(void)
{
  IDTR *idtr = NULL;
  IDT_ENTRY *entry = NULL;
  int i = 0;

  idtr = (IDTR *) va(IDTR_START_ADDRESS);
  entry = (IDT_ENTRY *) (va(IDTR_START_ADDRESS + sizeof(IDTR)));
  idtr->base_address = (QWORD) entry;
  idtr->limit = IDT_TABLE_SIZE - 1;
#if 0

  set_idt_entry(&entry[0], isr_divide_error, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[1], isr_debug, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[2], isr_nmi, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[3], isr_break_point, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[4], isr_overflow, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[5], isr_bound_range_exceeded, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[6], isr_invalid_opcode, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[7], isr_device_not_available, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[8], isr_double_fault, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[9], isr_coprocessor_segment_overrun, 0x08,
               IDT_FLAGS_IST0, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[10], isr_invalid_tss, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[11], isr_segment_not_present, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[12], isr_stack_segment_fault, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[13], isr_general_protection, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[14], isr_page_fault, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[15], isr_15, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[16], isr_fpu_error, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[17], isr_alignment_check, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[18], isr_machine_check, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[19], isr_simd_error, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[20], isr_etc_exception, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

  for (i = 21; i < 32; i++) {
    set_idt_entry(&entry[i], isr_etc_exception, 0x08, IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  }
#else
  set_idt_entry(&entry[0], isr_divide_error, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[1], isr_debug, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[2], isr_nmi, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[3], isr_break_point, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[4], isr_overflow, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[5], isr_bound_range_exceeded, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[6], isr_invalid_opcode, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[7], isr_device_not_available, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[8], isr_double_fault, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[9], isr_coprocessor_segment_overrun, 0x08,
               IDT_FLAGS_IST0, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[10], isr_invalid_tss, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[11], isr_segment_not_present, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[12], isr_stack_segment_fault, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[13], isr_general_protection, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[14], isr_page_fault, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[15], isr_15, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[16], isr_fpu_error, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[17], isr_alignment_check, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[18], isr_machine_check, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[19], isr_simd_error, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[20], isr_etc_exception, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

  for (i = 21; i < 32; i++) {
    set_idt_entry(&entry[i], isr_etc_exception, 0x08, IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  }
#endif

  set_idt_entry(&entry[32], isr_timer, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[34], isr_slave_pic, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[35], isr_serial2, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[36], isr_serial1, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[37], isr_parallel2, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[38], isr_floppy, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[39], isr_parallel1, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[40], isr_rtc, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[41], isr_reserved, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[42], isr_not_used1, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[43], isr_not_used2, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[44], isr_mouse, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[45], isr_coprocessor, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[46], isr_hdd1, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[47], isr_hdd2, 0x08, IDT_FLAGS_IST1,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  set_idt_entry(&entry[48], isr_lapic_timer, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

  set_idt_entry(&entry[49], isr_ipi, 0x08, IDT_FLAGS_IST0,
               IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

  for (i = 50; i < IDT_MAX_ENTRY_COUNT; i++) {
    set_idt_entry(&entry[i], isr_etc_interrupt, 0x08, IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
  }
}
