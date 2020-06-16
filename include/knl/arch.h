#ifndef __ARCH_H__
#define __ARCH_H__

#define BOOT_ADDR                   (0x94000)

#define META_OFFSET                 (0x800)

#define KERNEL_ADDR                 (0x300000)    // 3MB (unit: B)
#define APP_ADDR                    (0xC800000)   // 200MB (unit: B)

#define UKID_OFFSET                 (0x00)
#define PML4_OFFSET                 (0x08)
#define APIC_OFFSET                 (0x10)
#define CPU_START_OFFSET            (0x18)
#define CPU_END_OFFSET		        (0x20)
#define MEMORY_START_OFFSET		    (0x28)
#define MEMORY_END_OFFSET		    (0x30)

// TO BE DELETED
#define MAX_LOG_COUNT               (16383)
#define LOG_LENGTH                  (64)

// Systemwide resource informaion
// Should be modified depend on the system
#define MAX_UNIKERNEL               (100)   // Theoretically total core - linux core
#define MAX_CORE                    (100)
#define MAX_MEMORY                  (100)
#define MAX_PAPIC_ID                (600)   // Maximum number of physical cores to wake

// Used only start with index
#define CPUS_PER_NODE               (24)
#define MEMORYS_PER_NODE            (10)

// Shared Memory Layout
#define UNIKERNEL_START             (128)
#define SHARED_MEMORY_SIZE          (3)     // 3GB (unit: GB), DO NOT MODIFY

#define SHARED_MEMORY_START         ((unsigned long) (UNIKERNEL_START - SHARED_MEMORY_SIZE) << 30)
#define APP_START_OFFSET            ((unsigned long) (2<<20))       // 2MB (0~2MB unavailable)
#define APP_SIZE                    ((unsigned long) (510<<20))     // 510MB (2MB~512MB)
#define IPC_START_OFFSET            (APP_START_OFFSET + APP_SIZE)
#define IPC_SIZE                    ((unsigned long) (512<<20))     // 512MB
#define CHANNEL_START_OFFSET        (IPC_START_OFFSET + IPC_SIZE) 
#define CHANNEL_SIZE                ((unsigned long) (1<<30))       // 1GB 
#define VCON_START_OFFSET           (CHANNEL_START_OFFSET + CHANNEL_SIZE)
#define STAT_START_OFFSET           (VCON_START_OFFSET + 0x1000 * MAX_UNIKERNEL) 
#define SHELL_STORAGE_START_OFFSET  (STAT_START_OFFSET + sizeof(STAT_AREA))
#define LOG_START_OFFSET            (SHELL_STORAGE_START_OFFSET + sizeof(SHELL_STORAGE_AREA))
#define LOG_SIZE                    ((MAX_LOG_COUNT+1) * LOG_LENGTH)

//38, 3a, 3c used in QEMU : total_count, kernel32, kernel64
#define QEMU_OFFSET             (0x3E)

#define CONFIG_UKID_ADDR        (BOOT_ADDR + META_OFFSET + UKID_OFFSET)
#define CONFIG_PML4_ADDR        (BOOT_ADDR + META_OFFSET + PML4_OFFSET)
#define CONFIG_APIC_ADDR        (BOOT_ADDR + META_OFFSET + APIC_OFFSET)
#define CONFIG_CPU_START        (BOOT_ADDR + META_OFFSET + CPU_START_OFFSET)
#define CONFIG_CPU_END          (BOOT_ADDR + META_OFFSET + CPU_END_OFFSET)
#define CONFIG_MEM_START        (BOOT_ADDR + META_OFFSET + MEMORY_START_OFFSET)
#define CONFIG_MEM_END          (BOOT_ADDR + META_OFFSET + MEMORY_END_OFFSET)
#define CONFIG_QEMU             (BOOT_ADDR + META_OFFSET + QEMU_OFFSET)

#endif  /* __ARCH_H__ */
