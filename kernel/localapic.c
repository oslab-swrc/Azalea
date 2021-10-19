// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localapic.h"
#include "macros.h"
#include "mpconfigtable.h"
#include "page.h"
#include "console.h"
#include "pit.h"
#include "irq.h"
#include "console.h"

static QWORD ticks_in_10ms = 0;
static QWORD ticks_in_1ms = 0;

extern QWORD g_memory_start;
extern QWORD g_cpu_start;

/**
 *
 */
void enable_software_local_apic(void)
{
  volatile unsigned long value;

  value = x2apic_read(APIC_REGISTER_SVR);
  x2apic_write(APIC_REGISTER_SVR, value | 0x100); 
}

/**
 *
 */
void disable_software_local_apic(void)
{
  volatile unsigned long value;

  value = x2apic_read(APIC_REGISTER_SVR);
  x2apic_write(APIC_REGISTER_SVR, value & 0xFEFF );
}

/**
 *
 */
void lapic_send_eoi(void)
{
  x2apic_write(APIC_REGISTER_EOI, 0);
}

/**
 *
 */
void set_task_priority(BYTE priority)
{
  x2apic_write(APIC_REGISTER_TASK_PRIORITY, (unsigned long) priority);
}

/**
 *
 */
void local_vector_table_init(void)
{
  volatile unsigned long value = 0;
	
  value = x2apic_read(APIC_REGISTER_TIMER);
  x2apic_write(APIC_REGISTER_TIMER, value | APIC_INTERRUPT_MASK);

  value = x2apic_read(APIC_REGISTER_LINT0);
  x2apic_write(APIC_REGISTER_LINT0, value | APIC_INTERRUPT_MASK);

  x2apic_write(APIC_REGISTER_LINT1, APIC_TRIGGER_MODE_EDGE| APIC_POLARITY_ACTIVE_HIGH | APIC_DELIVERY_MODE_NMI);
  value = x2apic_read(APIC_REGISTER_ERROR);
  x2apic_write(APIC_REGISTER_ERROR, value | APIC_INTERRUPT_MASK);

  value = x2apic_read(APIC_REGISTER_PERFORMANCE_MONITORING_COUNTER);
  x2apic_write(APIC_REGISTER_PERFORMANCE_MONITORING_COUNTER, value | APIC_INTERRUPT_MASK) ;

   value = x2apic_read(APIC_REGISTER_THERMAL_SENSOR);
   x2apic_write(APIC_REGISTER_THERMAL_SENSOR, value | APIC_INTERRUPT_MASK);
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
  QWORD curr_cnt = 0;
  QWORD value = 0 ;

  x2apic_write(APIC_REGISTER_TIMER_DIV, 0x3) ;
  x2apic_write(APIC_REGISTER_TIMER_INIT_CNT, 0xFFFFFFFF) ;

  // Delay
  wait_using_directPIT(MSTOCOUNT(10));

  curr_cnt = x2apic_read(APIC_REGISTER_TIMER_CURR_CNT);
 
  value = x2apic_read(APIC_REGISTER_TIMER) ;
  x2apic_write(APIC_REGISTER_TIMER, value | APIC_INTERRUPT_MASK ) ;

  ticks_in_10ms = 0xFFFFFFFF - curr_cnt ;
} 

/*
 *
 */
void lapic_start_timer(int msec)
{
  QWORD ticks = (ticks_in_10ms / 10) * msec;

  if (ticks > 0xFFFFFFFF)
    ticks = 0xFFFFFFFF;

  x2apic_write(APIC_REGISTER_TIMER_DIV, 0x3) ;
  x2apic_write(APIC_REGISTER_TIMER,  48 | APIC_TIMER_MODE_PERIODIC ) ;
  x2apic_write(APIC_REGISTER_TIMER_INIT_CNT, ticks)  ;     // ticks must 32bit

  lk_print_xy(0,19,"lapic_start_timer\n") ;
}

/*
 *
 */
void lapic_start_timer_oneshot(int msec)
{
  QWORD ticks = (ticks_in_10ms / 10) * msec;

  if (ticks > 0xFFFFFFFF)
    ticks = 0xFFFFFFFF;

  x2apic_write(APIC_REGISTER_TIMER_DIV, 0x3) ;
  x2apic_write(APIC_REGISTER_TIMER,  48 | APIC_TIMER_MODE_ONESHOT ) ;
  x2apic_write(APIC_REGISTER_TIMER_INIT_CNT , ticks ) ;     // ticks must 32bit

}

/*
 *
 */
void lapic_stop_timer(void)
{
  x2apic_write(APIC_REGISTER_TIMER_INIT_CNT, 0);
}

/*
 *
 */
void lapic_consumed_time_slice(long *time_slice, long *time_tick)
{
  DWORD init_cnt = 0, curr_cnt = 0;

  init_cnt = x2apic_read(APIC_REGISTER_TIMER_INIT_CNT) ;
  curr_cnt = x2apic_read(APIC_REGISTER_TIMER_CURR_CNT) ;

  *time_tick = (long) (init_cnt - curr_cnt);
  *time_slice = *time_tick / lapic_get_ticks_in_1ms();
}


/*
 *
 */
int lapic_timer_interrupt_pending(void)
{
  DWORD delivery_status = 0;

  delivery_status = x2apic_read( APIC_REGISTER_TIMER ) ;
  delivery_status &= APIC_DELIVERY_STATUS_PENDING;

  return delivery_status;
}

/*
 *
 */
int lapic_send_ipi(BYTE dest_shorthand, BYTE dest, BYTE irq)
{
  DWORD tmp_dest = 0;
  DWORD tmp_dest_shorthand = 0;

  BYTE flags = irq_nested_disable();

//TODO
  tmp_dest = (DWORD) dest + g_cpu_start;
  tmp_dest_shorthand = (DWORD) dest_shorthand;

  if (dest_shorthand == 0x0) {
    	x2apic_write(APIC_REGISTER_ICR_UPPER, tmp_dest << 24 ) ;
  }

  x2apic_write(APIC_REGISTER_ICR_LOWER, 
      tmp_dest_shorthand << 18 | APIC_TRIGGER_MODE_EDGE |
      APIC_LEVEL_ASSERT | APIC_DESTINATION_MODE_PHYSICAL |
      APIC_DELIVERY_MODE_FIXED | irq ) ;

  irq_nested_enable(flags);

  return 0;
}
