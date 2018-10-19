#include "entry.h"

.text
.code64

.extern lk_common_exception_handler, lk_common_interrupt_handler
.extern timer_handler, pagefault_handler, ipi_handler

.global isr_divide_error, isr_debug, isr_nmi, isr_break_point, isr_overflow
.global isr_bound_range_exceeded, isr_invalid_opcode, isr_device_not_available, isr_double_fault
.global isr_fpu_error, isr_alignment_check, isr_machine_check, isr_simd_error, isr_etc_exception
.global isr_coprocessor_segment_overrun, isr_invalid_tss, isr_segment_not_present
.global isr_stack_segment_fault, isr_general_protection, isr_page_fault, isr_15

.global isr_timer, isr_slave_pic, isr_serial2, isr_serial1, isr_parallel2
.global isr_floppy, isr_parallel1, isr_rtc, isr_reserved, isr_not_used1, isr_not_used2
.global isr_mouse, isr_coprocessor, isr_hdd1, isr_hdd2, isr_lapic_timer, isr_etc_interrupt
.global isr_ipi

.global lk_general_protection_handler

//0, Divide Error ISR
isr_divide_error: 
  SAVECONTEXT
  movq $0,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//1, Debug ISR
isr_debug: 
  SAVECONTEXT
  movq $1,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//2, NMI ISR
isr_nmi: 
  SAVECONTEXT
  movq $2,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//3, BreakPoint ISR
isr_break_point: 
  SAVECONTEXT
  movq $3,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//4, Overflow ISR
isr_overflow: 
  SAVECONTEXT
  movq $4,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//5, Bound Range Exceeded ISR
isr_bound_range_exceeded: 
  SAVECONTEXT
  movq $5,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//6, Invalid Opcode ISR
isr_invalid_opcode: 
  SAVECONTEXT
  movq $6,%rdi
  movq 8(%rbp),%rsi
  movq %rbp,%rdx
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//7, Device Not Available ISR
isr_device_not_available: 
  SAVECONTEXT
  movq $7,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//8, Double Fault ISR
isr_double_fault: 
  SAVECONTEXT
  movq $8,%rdi
  movq 8(%rbp),%rsi
  call lk_common_exception_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//9, Coprocessor Segment Overrun ISR
isr_coprocessor_segment_overrun: 
  SAVECONTEXT
  movq $9,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//10, Invalid TSS ISR
isr_invalid_tss: 
  SAVECONTEXT
  movq $10,%rdi
  movq 8(%rbp),%rsi
  call lk_common_exception_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//11, Segment Not Present ISR
isr_segment_not_present:  
  SAVECONTEXT
  movq $11,%rdi
  movq 8(%rbp),%rsi
  call lk_common_exception_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//12, Stack Segment Fault ISR
isr_stack_segment_fault:  
  SAVECONTEXT
  movq $12,%rdi
  movq 8(%rbp),%rsi
  call lk_common_exception_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//13, General Protection ISR
isr_general_protection: 
  SAVECONTEXT
  movq $13,%rdi
  movq 8(%rbp),%rsi           // error code
  movq 16(%rbp),%rdx          // rip
#ifdef _qemu_
  call lk_general_protection_handler
#else
  call lk_common_exception_handler
#endif
  LOADCONTEXT
  addq $8,%rsp
  iretq

//14, page Fault ISR
isr_page_fault: 
  SAVECONTEXT
  movq %cr2, %rdi         // fault address
  movq 8(%rbp),%rsi       // error code
  movq 16(%rbp),%rdx      // rip
  call pagefault_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//15, Reserved ISR
isr_15: 
  SAVECONTEXT
  movq $15,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//16, FPU Error ISR
isr_fpu_error: 
  SAVECONTEXT
  movq $16,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//17, Alignment Check ISR
isr_alignment_check: 
  SAVECONTEXT
  movq $17,%rdi
  movq 8(%rbp),%rsi
  call lk_common_exception_handler
  LOADCONTEXT
  addq $8,%rsp
  iretq

//18, Machine Check ISR
isr_machine_check: 
  SAVECONTEXT
  movq $18,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//19, SIMD Floating Point Exception ISR
isr_simd_error: 
  SAVECONTEXT
  movq $19,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//20~31, Reserved ISR
isr_etc_exception: 
  SAVECONTEXT
  movq $20,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

/*
 Interrupt handler
*/
//32, Timer ISR
.extern timer_handler

isr_timer: 
  SAVECONTEXT
  movq $32,%rdi
  movq 8(%rbp),%rsi    // rip
  call timer_handler
  LOADCONTEXT
  iretq

//34, Slave PIC ISR
isr_slave_pic: 
  SAVECONTEXT
  movq $34,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//35, Serial port 2 ISR
isr_serial2: 
  SAVECONTEXT
  movq $35,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//36, Serial port 1 ISR
isr_serial1: 
  SAVECONTEXT
  movq $36,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//37, Parallel port 2 ISR
isr_parallel2: 
  SAVECONTEXT
  movq $37,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//38, Floppy disk controller ISR
isr_floppy: 
  SAVECONTEXT
  movq $38,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//39, Parallel port 1 ISR
isr_parallel1: 
  SAVECONTEXT
  movq $39,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//40, RTC ISR
isr_rtc: 
  SAVECONTEXT
  movq $40,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//41, Reserved
isr_reserved: 
  SAVECONTEXT
  movq $41,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//42, Not used
isr_not_used1: 
  SAVECONTEXT
  movq $42,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//43, Not used
isr_not_used2: 
  SAVECONTEXT
  movq $43,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//44, Mouse
isr_mouse: 
  SAVECONTEXT
  movq $44,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//45, Coprocessor 
isr_coprocessor: 
  SAVECONTEXT
  movq $45,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//46, Hard disk 1
isr_hdd1: 
  SAVECONTEXT
  movq $46,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//47, Hard disk 2
isr_hdd2: 
  SAVECONTEXT
  movq $47,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq

//48, Local APIC timer
isr_lapic_timer: 
  SAVECONTEXT
  movq $48,%rdi
  movq 8(%rbp),%rsi    //rip
  call timer_handler
  LOADCONTEXT
  iretq

//49, Inter-processor interrupt
isr_ipi: 
  SAVECONTEXT
  movq $49,%rdi
  movq 8(%rbp),%rsi    //rip
  call ipi_handler
  LOADCONTEXT
  iretq


//50, Others
isr_etc_interrupt: 
  SAVECONTEXT
  movq $50,%rdi
  call lk_common_exception_handler
  LOADCONTEXT
  iretq
