/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __IPOCAP_H__
#define __IPOCAP_H__

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

extern QWORD g_power_limit_mw;
extern QWORD g_power_base_mw;
extern QWORD g_rapl_energy_unit;

#define TSC_KHZ 1496534 //this value is for KNL, add calibration code
#define tsc_to_ms(tsc) 	(( tsc )/TSC_KHZ)

static inline QWORD __rdtsc(void){
	unsigned long low, high;

	asm volatile("rdtsc" : "=a" (low), "=d" (high));

	return (QWORD) ((low) | (high) << 32);
}

static inline DWORD read_energy_counter(void){
    unsigned long low, high;

	asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (MSR_PKG_ENERGY_STATUS));

	return (DWORD) low;
}

/*
 * Energy Status Units (bits 12:8 of MSR_RAPL_POWER_UNIT Register)
 * 1 counter = 1/2^ESU Joule
 */
static inline unsigned int read_energy_unit(void){
    unsigned long low, high;

    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (MSR_RAPL_POWER_UNIT));

	return (unsigned int) (low & 0x1f00) >> 8;
}

static inline QWORD rapl_energy_to_uj(QWORD counter){
	unsigned int energy_unit = g_rapl_energy_unit;
	return (counter * 1000000) >> energy_unit;
}

/*
 * referred to arch/x86/include/asm/mwait.h of Linux kernel
 */

#define MWAIT_ECX_INTERRUPT_BREAK	0x1
#define MWAITX_ECX_TIMER_ENABLE		0x1
#define MWAITX_DISABLE_CSTATES		0xf

struct cpuidle_state {
	unsigned int	flags;
	unsigned int	exit_latency; /* in US */
	unsigned int	target_residency; /* in US */
};

static inline void __monitor(const void *eax, unsigned long ecx,
			     unsigned long edx)
{
	/* "monitor %eax, %ecx, %edx;" */
	asm volatile(".byte 0x0f, 0x01, 0xc8;"
		     :: "a" (eax), "c" (ecx), "d"(edx));
}

static inline void __mwait(unsigned long eax, unsigned long ecx)
{
	/* "mwait %eax, %ecx;" */
	asm volatile(".byte 0x0f, 0x01, 0xc9;"
		     :: "a" (eax), "c" (ecx));
}


#endif
