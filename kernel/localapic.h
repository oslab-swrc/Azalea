#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include "az_types.h"

#define APIC_REGISTER_EOI				  0x0000B0
#define APIC_REGISTER_SVR				  0x0000F0
#define APIC_REGISTER_APIC_ID				  0x000020
#define APIC_REGISTER_TASK_PRIORITY			  0x000080
#define APIC_REGISTER_TIMER				  0x000320
#define APIC_REGISTER_THERMAL_SENSOR			  0x000330
#define APIC_REGISTER_PERFORMANCE_MONITORING_COUNTER	  0x000340
#define APIC_REGISTER_LINT0				  0x000350
#define APIC_REGISTER_LINT1				  0x000360
#define APIC_REGISTER_ERROR				  0x000370
#define APIC_REGISTER_ICR_LOWER				  0x000300
#define APIC_REGISTER_ICR_UPPER				  0x000310
#define APIC_REGISTER_TIMER_DIV				  0x0003E0
#define APIC_REGISTER_TIMER_INIT_CNT			  0x000380
#define APIC_REGISTER_TIMER_CURR_CNT			  0x000390

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
