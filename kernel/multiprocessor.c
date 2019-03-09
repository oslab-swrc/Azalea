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

static int list_apic_id[288] = {0, };
static int curr_cpu = 0;

#define IA32_APIC_BASE_MSR		0x0000001B
#define MSR_APIC_BASE			0x00000800
#define MSR_LOCAL_APIC_ID		0x00000002

#define APIC_BASE_ADDR			0xfee00000

#define MSR_XAPIC_ENABLE                        (1UL << 11)
#define MSR_X2APIC_ENABLE                       (1UL << 10)

typedef unsigned int uint32_t ;
typedef unsigned long uint64_t;

inline static uint64_t rdmsr_my(uint32_t msr) 
{
  uint32_t low, high;

  asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

  return ((uint64_t)high << 32) | low;
}

inline static void wrmsr_my(uint32_t msr, uint64_t value)
{
  uint32_t low =  (uint32_t) value;
  uint32_t high = (uint32_t) (value >> 32);

  asm volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high) : "memory");
}
/*
inline static QWORD rdmsr_apic(unsigned int msr) {

  unsigned int low, high;

  asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

  return ((QWORD)high << 32) | low;
}
*/

void disable_x2apic()
{
  QWORD msr ;
  msr = rdmsr_my(IA32_APIC_BASE_MSR);

  if (!(msr & MSR_X2APIC_ENABLE))
    return ;

  wrmsr_my(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE | MSR_XAPIC_ENABLE));
  wrmsr_my(IA32_APIC_BASE_MSR, msr & ~(MSR_X2APIC_ENABLE)) ;
}

/**
 * Set physical APIC ID to the list with sequential number,
 * then return the logical CPU ID. 
 */
BYTE set_apic_id(BYTE apicid)
{
  list_apic_id[apicid] = curr_cpu++;

  return curr_cpu;
}

/*
 * get physical APIC ID
 */
BYTE get_papic_id(void)
{
  MP_CONFIGURATION_TABLE_HEADER *mp_header = NULL;
  QWORD local_apic_base_address = 0;

  if (g_apic_id_address == 0) {
    if (get_mp_config_manager()->mp_configuration_table_header == NULL)
      return 0;
    mp_header = (MP_CONFIGURATION_TABLE_HEADER *)va((unsigned long)get_mp_config_manager()->mp_configuration_table_header);
    local_apic_base_address = mp_header->memory_map_io_address_of_local_apic;
    g_apic_id_address = local_apic_base_address + APIC_REGISTER_APIC_ID;
  }

  return (*((DWORD *) va_apic(g_apic_id_address)) >> 24);
}

/*
 * get logical APIC ID
 */
BYTE get_apic_id(void)
{
  BYTE papic_id = get_papic_id();

  return list_apic_id[papic_id];
}

/*
 * start AP
 */
BOOL startup_ap(void)
{
  if (analysis_mp_config_table() == FALSE)
    return FALSE;

  enable_globallocalapic();

  // Enabling Local APIC of BSP
  enable_software_local_apic();

  lapic_cal_ticks_in_10ms();

  return TRUE;
}
