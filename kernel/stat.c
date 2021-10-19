// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/lock.h>
#include "stat.h"
#include "console.h"
#include "arch.h"
#include "memory_config.h"

// Global unikernel id
extern int g_ukid;

static STAT_AREA *g_stat_area;
static spinlock_t stat_lock;

/**
 * @brief Assign g_stat_area to specific memory address
 * @param none
 * @return none
 */
void stat_init(void)
{
  g_stat_area = (STAT_AREA *) (CONFIG_SHARED_MEMORY + STAT_START_OFFSET); 
}

/**
 * @brief Store used memory size in stat memory
 * @param mem_size - used memory size
 * @return none
 */
void set_mem_info(QWORD mem_size)
{
  spinlock_lock(&stat_lock);

  // Store used memory in stat memory
  g_stat_area->ukernel[g_ukid].mem_used = mem_size;

  spinlock_unlock(&stat_lock);
}

/**
 * @brief Store used physical cpu number for unikenrel in stat memory
 * @param core_num - used cpu number
 * @return none
 */
void set_cpu_num(int core_num)
{
  spinlock_lock(&stat_lock);

  // Store used physical cpu number in list 'core'
  // and increase total_core_num at the same time
  g_stat_area->ukernel[g_ukid].core[g_stat_area->ukernel[g_ukid].total_core_num++] = core_num; 

  spinlock_unlock(&stat_lock);
}

/**
 * @brief Store load of the input core(pid) in stat memory
 * @param pid - physical number of core
 * @param core_load - load of the core
 * @return none
 */
void set_cpu_load(int pid, QWORD core_load)
{
  spinlock_lock(&stat_lock);

  // Store the load of the core in stat memory
  g_stat_area->core_load[pid] = core_load;

  spinlock_unlock(&stat_lock);
}
