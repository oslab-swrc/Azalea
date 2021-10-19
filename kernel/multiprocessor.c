// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assemblyutility.h"
#include "localapic.h"
#include "mpconfigtable.h"
#include "multiprocessor.h"
#include "page.h"
#include "utility.h"
#include "console.h"
#include "debug.h"
#include <sys/lock.h>

int g_ap_ready = 0;
volatile int g_ap_count = 0;
volatile QWORD g_apic_id_address = 0;
int g_cpu_start;
int g_cpu_end;
int g_cpu_size;
int g_ukid;

extern QWORD g_memory_start;

static int list_apic_id[MAX_PAPIC_ID] = {-1, };
static int curr_cpu = 0;
static spinlock_t apic_lock;

/**
 * @brief Enabling X2APIC
 * @param none
 * @return none
 */
void enable_x2apic(void)
{
  QWORD msr;

  msr = x2apic_read(IA32_APIC_BASE_MSR);

  if (msr & MSR_X2APIC_ENABLE)
    return;

  x2apic_write(IA32_APIC_BASE_MSR, msr | MSR_X2APIC_ENABLE);
}

/**
 * @brief Disabling X2APIC
 * @param none
 * @return none
 */
void disable_x2apic(void)
{
  QWORD msr;
  msr = x2apic_read(IA32_APIC_BASE_MSR);

  if (!(msr & MSR_X2APIC_ENABLE))
    return;

  x2apic_write(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE | MSR_XAPIC_ENABLE));
  x2apic_write(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE)) ;
}

/**
 * @brief Set physical APIC ID to the list with sequential number,
 * @brief then return the logical CPU ID. 
 * @param apicid - physical number of core
 * @return Logical core number assigned 
 */
QWORD set_apic_id(QWORD apicid)
{
  spinlock_lock(&apic_lock);
  list_apic_id[apicid] = curr_cpu++;
  spinlock_unlock(&apic_lock);

  return curr_cpu;
}

/**
 * @brief Get physical APIC ID
 * @param none
 * @return Current physical core number
 */
QWORD get_papic_id(void)
{
  return x2apic_read(APIC_REGISTER_APIC_ID) ;
}

/**
 * @brief Get logical APIC ID
 * @param none
 * @return Current logical core number
 */
QWORD get_apic_id(void)
{
  QWORD papic_id = get_papic_id();

  return list_apic_id[papic_id];
}

/**
 * @brief Startup AP cores
 * @param none
 * @return success (0), fail (-1)
 */
BOOL startup_ap(void)
{
  if (analysis_mp_config_table() == FALSE)
    return FALSE;

  // Enabling Local APIC of BSP
  enable_software_local_apic();

  lapic_cal_ticks_in_10ms();

  return TRUE;
}

/**
 * @brief Return the number of cores
 * @param none
 * @return Starting core number
 */
int get_cpu_num(void)
{
  return g_cpu_start;
}
