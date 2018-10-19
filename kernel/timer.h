#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"

// Funtions
void timer_init(void);
void timer_handler(int irq_no, QWORD rip);

size_t sys_get_ticks(void);
void sys_msleep(unsigned int ms);

#endif  /* __TIMER_H__ */
