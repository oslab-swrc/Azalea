#include "localapic.h"
#include "macros.h"
#include "mpconfigtable.h"
#include "page.h"
#include "console.h"
#include "pit.h"
#include "irq.h"
#include "console.h"

static QWORD local_apic_base_address = 0;
static QWORD ticks_in_10ms = 0;
static QWORD ticks_in_1ms = 0;

extern QWORD g_memory_start;
extern QWORD g_cpu_start;

#define get_local_apic_base_address() ({ \
  if (unlikely(local_apic_base_address == 0)) { \
    MP_CONFIGURATION_TABLE_HEADER *mp_header; \
    mp_header = get_mp_config_manager()->mp_configuration_table_header; \
    mp_header = (MP_CONFIGURATION_TABLE_HEADER *)va(mp_header); \
    local_apic_base_address = va_apic(mp_header->memory_map_io_address_of_local_apic); \
  } \
  local_apic_base_address; \
})

/*
 * enable software local APIC
 */
void enable_software_local_apic(void)
{
  QWORD local_apic_base_address = 0;

  local_apic_base_address = get_local_apic_base_address();
  *(DWORD *) (local_apic_base_address + APIC_REGISTER_SVR) |= 0x100;
}

/*
 * disable software local APIC
 */
void disable_software_local_apic(void)
{
  QWORD local_apic_base_address = 0;

  local_apic_base_address = get_local_apic_base_address();
  *(DWORD *) (local_apic_base_address + APIC_REGISTER_SVR) &= 0xFEFF;
}


/*
 * send end of local APIC interrupt 
 */
void lapic_send_eoi(void)
{
  QWORD local_apic_base_address = 0;

  local_apic_base_address = get_local_apic_base_address();
  //printk("LocalAPIC base addr = %x\n", local_apic_base_address);
  *(DWORD *) (local_apic_base_address + APIC_REGISTER_EOI) = 0;
}

/*
 * setting task priority
 */
void set_task_priority(BYTE priority)
{
  QWORD local_apic_base_address = 0;

  local_apic_base_address = get_local_apic_base_address();
  *(DWORD *) (local_apic_base_address + APIC_REGISTER_TASK_PRIORITY) = priority;
}

/*
 * initialize local vector table
 */
void local_vector_table_init(void)
{
  QWORD local_apic_base_address = 0;

  local_apic_base_address = get_local_apic_base_address();

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_LINT0) |= APIC_INTERRUPT_MASK;

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_LINT1) = APIC_TRIGGER_MODE_EDGE | APIC_POLARITY_ACTIVE_HIGH | APIC_DELIVERY_MODE_NMI;

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_ERROR) |= APIC_INTERRUPT_MASK;

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_PERFORMANCE_MONITORING_COUNTER) |= APIC_INTERRUPT_MASK;

  *(DWORD *) (local_apic_base_address + APIC_REGISTER_THERMAL_SENSOR) |= APIC_INTERRUPT_MASK;
}

/*
 * calculate local APIC ticks in 1 ms
 */
QWORD lapic_get_ticks_in_1ms(void)
{
  if (unlikely(ticks_in_1ms == 0))
    ticks_in_1ms = ticks_in_10ms / 10;
  return ticks_in_1ms;
}

/*
 * calculate local APIC ticks in 10 ms
 */
void lapic_cal_ticks_in_10ms(void)
{
  QWORD lapic_base = 0;
  QWORD curr_cnt = 0;

  lapic_base = get_local_apic_base_address();

  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_DIV) = 0xB;
  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_INIT_CNT) = 0xFFFFFFFF;

  // Delay
  wait_using_directPIT(MSTOCOUNT(10));

  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;

  // Calculate ticks in 10ms
  curr_cnt = *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_CURR_CNT);
  ticks_in_10ms = 0xFFFFFFFF - curr_cnt;
}

/*
 * start periodic local APIC timer
 */
void lapic_start_timer(int msec)
{
  QWORD lapic_base = 0;
  QWORD ticks = (ticks_in_10ms / 10) * msec;

  if (ticks > 0xFFFFFFFF)
    ticks = 0xFFFFFFFF;

  lapic_base = get_local_apic_base_address();

  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_DIV) = 0xB;
  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER) = 48 | APIC_TIMER_MODE_PERIODIC;
  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_INIT_CNT) = ticks;
}

/**
 * start oneshot local APIC timer
 */
void lapic_start_timer_oneshot(int msec)
{
  QWORD lapic_base = 0;
  QWORD ticks = (ticks_in_10ms / 10) * msec;

  if (ticks > 0xFFFFFFFF)
    ticks = 0xFFFFFFFF;

  lapic_base = get_local_apic_base_address();

  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_DIV) = 0xB;
  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER) = 48 | APIC_TIMER_MODE_ONESHOT;
  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_INIT_CNT) = (DWORD) ticks;
}

/**
 * stop local APIC timer - initialize TIMER_INITCNT
 */
void lapic_stop_timer(void)
{
  QWORD lapic_base = 0;

  lapic_base = get_local_apic_base_address();

  *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_INIT_CNT) = 0;
}

/*
 * calculate Consumed time slice and time tick
 */
void lapic_consumed_time_slice(long *time_slice, long *time_tick)
{
  QWORD lapic_base = 0;
  DWORD init_cnt = 0, curr_cnt = 0;

  lapic_base = get_local_apic_base_address();

  init_cnt = *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_INIT_CNT);
  curr_cnt = *(DWORD *) (lapic_base + APIC_REGISTER_TIMER_CURR_CNT);

  *time_tick = (long) (init_cnt - curr_cnt);
  *time_slice = *time_tick / lapic_get_ticks_in_1ms();
}


/*
 * pending local APIC timer interrupt
 */
int lapic_timer_interrupt_pending(void)
{
  QWORD lapic_base = 0;
  DWORD delivery_status = 0;

  lapic_base = get_local_apic_base_address();

  delivery_status = *(DWORD *) (lapic_base + APIC_REGISTER_TIMER);
  delivery_status &= APIC_DELIVERY_STATUS_PENDING;

  return delivery_status;
}

/*
 * send IPI of local APIC
 */
int lapic_send_ipi(BYTE dest_shorthand, BYTE dest, BYTE irq)
{
  QWORD lapic_base = 0;
  DWORD tmp_dest = 0;
  DWORD tmp_dest_shorthand = 0;

  BYTE flags = irq_nested_disable();

  tmp_dest = (DWORD) dest + g_cpu_start;
  tmp_dest_shorthand = (DWORD) dest_shorthand;

  lapic_base = get_local_apic_base_address();
  if (dest_shorthand == 0x0) {
    *(DWORD *) (lapic_base + APIC_REGISTER_ICR_UPPER) = tmp_dest << 24;
  }

  *(DWORD *) (lapic_base + APIC_REGISTER_ICR_LOWER) =
      tmp_dest_shorthand << 18 | APIC_TRIGGER_MODE_EDGE |
      APIC_LEVEL_ASSERT | APIC_DESTINATION_MODE_PHYSICAL |
      APIC_DELIVERY_MODE_FIXED | irq;

  irq_nested_enable(flags);

  return 0;
}
