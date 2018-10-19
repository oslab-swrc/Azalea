#ifndef __ARCH_H__
#define __ARCH_H__

#define BOOT_ADDR		(0x94000)

#define META_OFFSET		(0x800)
#define VCON_INDEX		(5)
#define VCON_SIZE		(6)

#define KERNEL_ADDR		(0x300000)		// 3MB (unit: B)
#define APP_ADDR		(0xC800000)		// 200MB (unit: B)

#define VCON_OFFSET			(0x00)
#define PML4_OFFSET			(0x08)
#define APIC_OFFSET			(0x10)
#define CPU_START_OFFSET		(0x18)
#define CPU_END_OFFSET			(0x20)
#define MEMORY_START_OFFSET		(0x28)
#define MEMORY_END_OFFSET		(0x30)

#define MAX_LOG_COUNT  (16383)
#define LOG_LENGTH     (64)

//38, 3a, 3c used in QEMU : total_count, kernel32, kernel64
#define QEMU_OFFSET                     (0x3E)

#define CONFIG_VCON_ADDR        (BOOT_ADDR + META_OFFSET + VCON_OFFSET)
#define CONFIG_PML4_ADDR        (BOOT_ADDR + META_OFFSET + PML4_OFFSET)
#define CONFIG_APIC_ADDR        (BOOT_ADDR + META_OFFSET + APIC_OFFSET)
#define CONFIG_CPU_START        (BOOT_ADDR + META_OFFSET + CPU_START_OFFSET)
#define CONFIG_CPU_END          (BOOT_ADDR + META_OFFSET + CPU_END_OFFSET)
#define CONFIG_MEM_START        (BOOT_ADDR + META_OFFSET + MEMORY_START_OFFSET)
#define CONFIG_MEM_END          (BOOT_ADDR + META_OFFSET + MEMORY_END_OFFSET)
#define CONFIG_QEMU             (BOOT_ADDR + META_OFFSET + QEMU_OFFSET)

#define CONFIG_SHELL_STORAGE    (100)

#endif  /* __ARCH_H__ */
