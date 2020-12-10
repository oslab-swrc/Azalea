/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __RAPL_H__
#define __RAPL_H__

#include "az_types.h"
/* 
 * Run Time Average Power Limiting (RAPL) Interface 
 * referred to arch/x86/include/asm/msr-index.h of Linux kernel
 */

#define MSR_RAPL_POWER_UNIT		0x00000606

#define MSR_PKG_POWER_LIMIT		0x00000610
#define MSR_PKG_ENERGY_STATUS	0x00000611
#define MSR_PKG_PERF_STATUS		0x00000613
#define MSR_PKG_POWER_INFO		0x00000614

#define MSR_DRAM_POWER_LIMIT	0x00000618
#define MSR_DRAM_ENERGY_STATUS	0x00000619
#define MSR_DRAM_PERF_STATUS	0x0000061b
#define MSR_DRAM_POWER_INFO		0x0000061c

#define MSR_PP0_POWER_LIMIT		0x00000638
#define MSR_PP0_ENERGY_STATUS	0x00000639
#define MSR_PP0_POLICY			0x0000063a
#define MSR_PP0_PERF_STATUS		0x0000063b

#define MSR_PP1_POWER_LIMIT		0x00000640
#define MSR_PP1_ENERGY_STATUS	0x00000641
#define MSR_PP1_POLICY			0x00000642


#define ARCH_STATIC_POWER   25  //watt:

extern DWORD g_rapl_energy_unit;
extern QWORD g_power_base_mw;

/*
 * Energy Status Units (bits 12:8 of MSR_RAPL_POWER_UNIT Register)
 * 1 counter = 1/2^ESU Joule
 */
static inline void rapl_init_energy_unit(void){
    DWORD low, high;

    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (MSR_RAPL_POWER_UNIT));

	g_rapl_energy_unit = (DWORD) ((low & 0x1f00) >> 8) ;
	g_power_base_mw = ARCH_STATIC_POWER * 1000;
	return;
}

static inline DWORD rapl_read_energy_counter(void){
    DWORD low, high;

	asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (MSR_PKG_ENERGY_STATUS));

	return (DWORD) low;
}

static inline QWORD rapl_energy_to_uj(QWORD counter){
	DWORD energy_unit = g_rapl_energy_unit;
	return (counter * 1000000) >> energy_unit;
}

static inline QWORD rapl_read_energy_uj(void) {
	DWORD low, high;
	
	asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (MSR_PKG_ENERGY_STATUS));
	
	return (low * 1000000) >> g_rapl_energy_unit;
}
#endif
