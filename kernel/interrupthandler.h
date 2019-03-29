#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "az_types.h"

void intr_handler_thread_init(void);
void lk_common_exception_handler(int vector_number, QWORD error_code, QWORD v);
void lk_common_interrupt_handler(int vector_number);
void pagefault_handler(QWORD fault_address, QWORD error_code, QWORD rip);
int register_interrupt(int irq);
int unregister_interrupt(int irq);

#endif /* __INTERRUPTHANDLER_H__ */
