#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "types.h"

#define MAX_PROCESSOR_COUNT		128	

BOOL startup_ap(void);
#if 0
QWORD get_apic_id(void);
#else
BYTE get_apic_id(void);
#endif

int get_core_num(void);

#endif  /* __MULTIPROCESSOR_H__ */
