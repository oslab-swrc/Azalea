#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__
#include "types.h"

#define ENTRY(name) \
  .global name; \
  .align 4,0x90; \
  name:

#ifndef __ASSEMBLY__
void load_gdtr(QWORD gdtr_address);
void load_tr(WORD tss_offset);
void load_idtr(QWORD idtr_address);

void enable_interrupt(void);
void disable_interrupt(void);

void enable_globallocalapic(void);

QWORD read_flags(void);

void read_msr(QWORD msr_address, QWORD * rdx, QWORD * rax);
void write_msr(QWORD msr_address, QWORD rdx, QWORD rax);

QWORD read_tsc(void);

QWORD get_cr3(void);
void set_cr3(QWORD data);

BOOL clear_bit(volatile BYTE * s, QWORD pos);
BOOL set_bit(volatile BYTE * s, QWORD pos);
BOOL toggle_bit(volatile BYTE * s, QWORD pos);

BYTE in_port_byte(WORD port);
void out_port_byte(WORD port, BYTE data);

void pause(void);
void hlt(void);
void nop(void);

#endif /* __ASSEMBLY__ */

#endif /*__ASSEMBLYUTILITY_H__*/
