/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MWAIT_H__
#define __MWAIT_H__

#include "az_types.h"


#define CSTATE_ARCH_HSW

/*
 * referred to arch/x86/include/asm/mwait.h 
 * and drivers/idle/intel_idle.c 
 * and include/linux/cpuidle.h of Linux kernel
 */

#define BIT(nr) (1UL << (nr))

#define MWAIT_ECX_INTERRUPT_BREAK	0x1
#define MWAITX_ECX_TIMER_ENABLE		0x1
#define MWAITX_DISABLE_CSTATES		0xf

/* Idle State Flags */
#define CPUIDLE_FLAG_NONE       	(0x00)
#define CPUIDLE_FLAG_POLLING		BIT(0) /* polling state */
#define CPUIDLE_FLAG_COUPLED		BIT(1) /* state applies to multiple cpus */
#define CPUIDLE_FLAG_TIMER_STOP 	BIT(2) /* timer is stopped on this state */
#define CPUIDLE_FLAG_UNUSABLE		BIT(3) /* avoid using this state */
#define CPUIDLE_FLAG_OFF		BIT(4) /* disable this state by default */
#define CPUIDLE_FLAG_TLB_FLUSHED	BIT(5) /* idle-state flushes TLBs */
#define CPUIDLE_FLAG_RCU_IDLE		BIT(6) /* idle-state takes care of RCU */
#define CPUIDLE_FLAG_ALWAYS_ENABLE	BIT(15)

#define MWAIT2flg(eax) ((eax & 0xFF) << 24)

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

#ifdef CSTATE_ARCH_KNL
static struct cpuidle_state knl_cstates[] = {
    {
        .flags = MWAITX_DISABLE_CSTATES,
        .exit_latency = 0,
        .target_residency = 0 },
    {
        //.name = "C1",
        //.desc = "MWAIT 0x00",
        .flags = 0x00,
        .exit_latency = 1,
        .target_residency = 2 },
    {
        //.name = "C6",
        //.desc = "MWAIT 0x10",
        .flags = 0x10,
        .exit_latency = 120,
        .target_residency = 500 },
    {
        //deepest cstate
        .flags = 0x10 }

};
static struct cpuidle_state * curr_arch = hsw_cstates;
#endif

#ifdef CSTATE_ARCH_HSW
static struct cpuidle_state hsw_cstates[] = {
    {
        .flags = MWAITX_DISABLE_CSTATES,
        .exit_latency = 0,
        .target_residency = 0 },
	{
		//.name = "C1",
		//.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 2,
		.target_residency = 2, },
	{
		//.name = "C1E",
		//.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01) | CPUIDLE_FLAG_ALWAYS_ENABLE,
		.exit_latency = 10,
		.target_residency = 20,},
	{
		//.name = "C3",
		//.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 33,
		.target_residency = 100,},
	{
		//.name = "C6",
		//.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 133,
		.target_residency = 400,},
	{
		//.name = "C7s",
		//.desc = "MWAIT 0x32",
		.flags = MWAIT2flg(0x32) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 166,
		.target_residency = 500,},
	{
		//.name = "C8",
		//.desc = "MWAIT 0x40",
		.flags = MWAIT2flg(0x40) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 300,
		.target_residency = 900, },
	{
		//.name = "C9",
		//.desc = "MWAIT 0x50",
		.flags = MWAIT2flg(0x50) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 600,
		.target_residency = 1800,},
	{
		//.name = "C10",
		//.desc = "MWAIT 0x60",
		.flags = MWAIT2flg(0x60) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 2600,
		.target_residency = 7700,},
	{
        //deepest cstate
        .flags = MWAIT2flg(0x60) | CPUIDLE_FLAG_TLB_FLUSHED }
};
static struct cpuidle_state * curr_arch = hsw_cstates;
#endif

static inline struct cpuidle_state * get_optimal_cstate(long msec) {
    int idx=0;
    struct cpuidle_state * cur, * next;
    
    do {
        cur = &(curr_arch[idx]);
        next = &(curr_arch[++idx]);
        if (cur->flags == next->flags) //deepest cstate
            break;
        
    } while ((next->exit_latency + next->target_residency) < msec);
    
    return cur;
}

#endif
