#ifndef __ISR_H__
#define __ISR_H__

void isr_divide_error(void);
void isr_debug(void);
void isr_nmi(void);
void isr_break_point(void);
void isr_overflow(void);
void isr_bound_range_exceeded(void);
void isr_invalid_opcode(void);
void isr_device_not_available(void);
void isr_double_fault(void);
void isr_coprocessor_segment_overrun(void);
void isr_invalid_tss(void);
void isr_segment_not_present(void);
void isr_stack_segment_fault(void);
void isr_general_protection(void);
void isr_page_fault(void);
void isr_15(void);
void isr_fpu_error(void);
void isr_alignment_check(void);
void isr_machine_check(void);
void isr_simd_error(void);
void isr_etc_exception(void);

// Interrupt handler ISR
void isr_timer(void);
void isr_slave_pic(void);
void isr_serial2(void);
void isr_serial1(void);
void isr_parallel2(void);
void isr_floppy(void);
void isr_parallel1(void);
void isr_rtc(void);
void isr_reserved(void);
void isr_not_used1(void);
void isr_not_used2(void);
void isr_mouse(void);
void isr_coprocessor(void);
void isr_hdd1(void);
void isr_hdd2(void);
void isr_lapic_timer(void);
void isr_ipi(void);
void isr_etc_interrupt(void);

#endif // __ISR_H__
