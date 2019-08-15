#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "az_types.h"

#define MAX_PROCESSOR_COUNT		288	

BOOL startup_ap(void);
QWORD get_papic_id(void);
QWORD get_apic_id(void);

int get_cpu_num(void);

#endif  /* __MULTIPROCESSOR_H__ */
