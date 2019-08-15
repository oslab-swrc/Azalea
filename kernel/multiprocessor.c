#include "assemblyutility.h"
#include "localapic.h"
#include "mpconfigtable.h"
#include "multiprocessor.h"
#include "page.h"
#include "utility.h"
#include "console.h"

int g_ap_ready = 0;
volatile int g_ap_count = 0;
volatile QWORD g_apic_id_address = 0;
int g_cpu_start;
int g_cpu_end;
int g_cpu_size;

extern QWORD g_memory_start;

static int list_apic_id[506] = {0, };
static int curr_cpu = 0;

/**
 *
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
 *
 */
void disable_x2apic()
{
  QWORD msr;
  msr = x2apic_read(IA32_APIC_BASE_MSR);

  if (!(msr & MSR_X2APIC_ENABLE))
    return;

  x2apic_write(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE | MSR_XAPIC_ENABLE));
  x2apic_write(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE)) ;
}

/**
 * Set physical APIC ID to the list with sequential number,
 * then return the logical CPU ID. 
 */
QWORD set_apic_id(QWORD apicid)
{
  list_apic_id[apicid] = curr_cpu++;

  return curr_cpu;
}

/*
 * get physical APIC ID
 */
QWORD get_papic_id(void)
{
  return x2apic_read(APIC_REGISTER_APIC_ID) ;
}

/*
 * get logical APIC ID
 */
QWORD get_apic_id(void)
{
  QWORD papic_id = get_papic_id();

  return list_apic_id[papic_id];
}

/*
 * start AP
 */
BOOL startup_ap(void)
{
  if (analysis_mp_config_table() == FALSE)
    return FALSE;

//  enable_globallocalapic();   // apic enabled in entrypoint.S 

  // Enabling Local APIC of BSP
  enable_software_local_apic();

  lapic_cal_ticks_in_10ms();

  return TRUE;
}

/**
 * Return the number of cores
 */
int get_cpu_num(void)
{
  return g_cpu_start;
}
