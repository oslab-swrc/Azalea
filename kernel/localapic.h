// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include "az_types.h"

//X2APIC
#define BASE_APIC_MSR					  0x00000800
#define APIC_REGISTER_EOI                                 (BASE_APIC_MSR+((0x0000B0)>>4))
#define APIC_REGISTER_SVR                                 (BASE_APIC_MSR+((0x0000F0)>>4))
#define APIC_REGISTER_APIC_ID                             (BASE_APIC_MSR+((0x000020)>>4))
#define APIC_REGISTER_TASK_PRIORITY                       (BASE_APIC_MSR+((0x000080)>>4))
#define APIC_REGISTER_TIMER                               (BASE_APIC_MSR+((0x000320)>>4))
#define APIC_REGISTER_THERMAL_SENSOR                      (BASE_APIC_MSR+((0x000330)>>4))
#define APIC_REGISTER_PERFORMANCE_MONITORING_COUNTER      (BASE_APIC_MSR+((0x000340)>>4))
#define APIC_REGISTER_LINT0                               (BASE_APIC_MSR+((0x000350)>>4))
#define APIC_REGISTER_LINT1                               (BASE_APIC_MSR+((0x000360)>>4))
#define APIC_REGISTER_ERROR                               (BASE_APIC_MSR+((0x000370)>>4))
#define APIC_REGISTER_ICR_LOWER                           (BASE_APIC_MSR+((0x000300)>>4))
#define APIC_REGISTER_ICR_UPPER                           (BASE_APIC_MSR+((0x000310)>>4))
#define APIC_REGISTER_TIMER_DIV                           (BASE_APIC_MSR+((0x0003E0)>>4))
#define APIC_REGISTER_TIMER_INIT_CNT                      (BASE_APIC_MSR+((0x000380)>>4))
#define APIC_REGISTER_TIMER_CURR_CNT                      (BASE_APIC_MSR+((0x000390)>>4))

#define IA32_APIC_BASE_MSR                    0x0000001B
#define MSR_XAPIC_ENABLE                      (1UL << 11)
#define MSR_X2APIC_ENABLE                     (1UL << 10)

// Delivery Mode
#define APIC_DELIVERY_MODE_FIXED			  0x000000
#define APIC_DELIVERY_MODE_LOWEST_PRIORITY		  0x000100
#define APIC_DELIVERY_MODE_SMI				  0x000200
#define APIC_DELIVERY_MODE_NMI				  0x000400
#define APIC_DELIVERY_MODE_INIT				  0x000500
#define APIC_DELIVERY_MODE_STARTUP			  0x000600
#define APIC_DELIVERY_MODE_EXT_INT			  0x000700

// Destination Mode
#define APIC_DESTINATION_MODE_PHYSICAL			  0x000000
#define APIC_DESTINATION_MODE_LOGICAL			  0x000800

// Delivery Mode
#define APIC_DELIVERY_STATUS_IDLE			  0x000000
#define APIC_DELIVERY_STATUS_PENDING			  0x001000

// Level
#define APIC_LEVEL_DEASSERT				  0x000000
#define APIC_LEVEL_ASSERT				  0x004000

// Trigger Mode
#define APIC_TRIGGER_MODE_EDGE				  0x000000
#define APIC_TRIGGER_MODE_LEVEL				  0x008000

// Destination Shorthand
#define APIC_DESTINATION_SHORT_HAND_NO_SHORT_HAND	  0x000000
#define APIC_DESTIANTION_SHORT_HAND_SELF		  0x040000
#define APIC_DESTINATION_SHORT_HAND_ALL_INCLUDING_SELF	  0x080000
#define APIC_DESTINATION_SHORT_HAND_ALL_EXCLUDING_SELF	  0x0C0000

// Interrupt Mask
#define APIC_INTERRUPT_MASK				  0x010000

// Timer Mode
#define APIC_TIMER_MODE_PERIODIC			  0x020000
#define APIC_TIMER_MODE_ONESHOT				  0x000000

// Interrupt Input Pin Polarity
#define APIC_POLARITY_ACTIVE_LOW			  0x002000
#define APIC_POLARITY_ACTIVE_HIGH			  0x000000

#ifndef __ASSEMBLY__

inline static QWORD x2apic_read(DWORD msr)
{
  DWORD low, high;

  asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

  return ((QWORD)high << 32) | low;
}

inline static void x2apic_write(DWORD msr, QWORD value)
{
  DWORD low =  (DWORD) value;
  DWORD high = (DWORD) (value >> 32);

  asm volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high) : "memory");
}

inline static void x2apic_write32(DWORD msr, DWORD high, DWORD low) 
{
  asm volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high) : "memory");	
}

/*
inline static DWORD safe_x2apic_wait_icr_idle()
{
	return 0;
}
*/

// Functions
QWORD get_local_apic_base_address(void);
void enable_software_local_apic(void);
void disable_software_local_apic(void);
void lapic_send_eoi(void);
void set_task_priority(BYTE bPriority);
void local_vector_table_init(void);
void lapic_cal_ticks_in_10ms(void);
void lapic_start_timer(int msec);
void lapic_start_timer_oneshot(int msec);
void lapic_stop_timer(void);
void lapic_consumed_time_slice(long *time_slice, long *time_tick);
QWORD lapic_get_ticks_in_1ms(void);
int lapic_timer_interrupt_pending(void);
int lapic_send_ipi(BYTE dest_shorthand, BYTE dest, BYTE irq);
#endif

#endif  /* __LOCALAPIC_H__ */
