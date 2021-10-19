// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __ARCH_H__
#define __ARCH_H__

#define BOOT_ADDR		(0x94000)

#define META_OFFSET		(0x800)

#define KERNEL_ADDR		(0x300000)		// 3MB (unit: B)
#define APP_ADDR		(0xC800000)		// 200MB (unit: B)

#define CPUS_PER_NODE      24
#define UNIKERNEL_START    48
#define MEMORYS_PER_NODE   10

//Fixed value for QEMU
#define VCON_ADDR	(0xb8000)
#define PML4_ADDR	(BOOT_ADDR+0x1000)
#define APIC_ADDR	(0xFEE00000)
#define CPU_START	(0)
#define CPU_END		(0)
#define MEMORY_START	(0x40000000)
#define MEMORY_END	(0x3C0000000)

#define UKID_OFFSET			(0x00)
#define PML4_OFFSET			(0x08)
#define APIC_OFFSET			(0x10)
#define CPU_START_OFFSET		(0x18)
#define CPU_END_OFFSET			(0x20)
#define MEMORY_START_OFFSET		(0x28)
#define MEMORY_END_OFFSET		(0x30)

#define TOTAL_COUNT			(0x38)
#define KERNEL32_COUNT			(0x3A)
#define KERNEL64_COUNT			(0x3C)
#define QEMU_OFFSET			(0x3E)

#define MAX_UNIKERNEL  (100)

#define MAX_LOG_COUNT  (16383)
#define LOG_LENGTH     (64)

#define CONFIG_UKID_ADDR		(BOOT_ADDR + META_OFFSET + UKID_OFFSET)
#define CONFIG_PML4_ADDR		(BOOT_ADDR + META_OFFSET + PML4_OFFSET)
#define CONFIG_APIC_ADDR		(BOOT_ADDR + META_OFFSET + APIC_OFFSET)
#define CONFIG_CPU_START		(BOOT_ADDR + META_OFFSET + CPU_START_OFFSET)
#define CONFIG_CPU_END			(BOOT_ADDR + META_OFFSET + CPU_END_OFFSET)
#define CONFIG_MEM_START		(BOOT_ADDR + META_OFFSET + MEMORY_START_OFFSET)
#define CONFIG_MEM_END			(BOOT_ADDR + META_OFFSET + MEMORY_END_OFFSET)

#define CONFIG_TOTAL_COUNT	        (BOOT_ADDR + META_OFFSET + TOTAL_COUNT)
#define CONFIG_KERNEL32	                (BOOT_ADDR + META_OFFSET + KERNEL32_COUNT)
#define CONFIG_KERNEL64	                (BOOT_ADDR + META_OFFSET + KERNEL64_COUNT)
#define CONFIG_QEMU	                (BOOT_ADDR + META_OFFSET + QEMU_OFFSET)

#define CONFIG_SHELL_STORAGE    (100)

#endif  /* __ARCH_H__ */
