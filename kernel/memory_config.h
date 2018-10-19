#ifndef	__MEM_CONFIG_H__
#define	__MEM_CONFIG_H__

#include "arch.h"

#define CONFIG_KERNEL_PAGETABLE_ADDRESS	(0x200000+g_memory_start)	// (unit: B)
#define CONFIG_KERNEL64_START		(0x300000+g_memory_start)	// (unit: B)
#define CONFIG_KERNEL64_START_VA	(CONFIG_KERNEL64_START_ADDR+CONFIG_PAGE_OFFSET)
#define CONFIG_KERNEL64_START_ADDR	(0x300000)
#define CONFIG_KERNEL_SIZE		(0x40000000)			// 1GB (unit: B)

#define CONFIG_HIGH_HALF_LIMIT		((QWORD)0xFFFF800000000000)
#define CONFIG_LOW_HALF_LIMIT		((QWORD)0x0000700000000000)

#define CONFIG_NUM_THREAD		2048

#define CONFIG_TCB_SIZE         	0x1000
#define CONFIG_PAGE_OFFSET      	0xFFFF8000C0000000

#define IDLE_THREAD_ADDRESS     	(0x800000+g_memory_start)
#define IDLE_THREAD_ADDRESS_VA		(0x800000+CONFIG_PAGE_OFFSET)

#define CONFIG_APP_ADDRESS          ((200<<20)+g_memory_start)		// 200MB (unit: B)

#define CONFIG_SHELL_STORAGE_AREA   ((CONFIG_SHELL_STORAGE<<20)+g_memory_start)

#define HEAP_START                  (0x40000000)        // 1GB (unit: B)
#define HEAP_SIZE                   (0x280000000)       // 10GB (unit: B)

#endif  /*__MEM_CONFIG_H__*/
