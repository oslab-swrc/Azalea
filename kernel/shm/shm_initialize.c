// Copyright (C) 2021 Portions Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: BSD-3-Clause

/****************************************************************************
 * mm/shm/shm_initialize.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <sys/lock.h>

#include "console_function.h"
#include "memory_config.h"
#include "shm.h"
#include "shm_mm_config.h"
#include "utility.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* State of the all shared memory */
struct shm_info_s *g_shminfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: shm_initialize
 *
 * Description:
 *   Perform one time, start-up initialization of the shared memor logic.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void shm_initialize(void)
{

  g_shminfo = (struct shm_info_s *) SHM_INFO_S_VA;

#ifdef DEBUG
  cs_printf("shm_initialize(): shminfo addr = 0x%q \n", g_shminfo);
#endif

  struct shm_region_s *region;
  int i;

  //spinlock_init(&g_shminfo->si_lock);

  /* Initialize each shared memory region */
  for (i = 0; i < CONFIG_ARCH_SHM_MAXREGIONS; i++)
  {
    region = &g_shminfo->si_region[i];
    lk_memset(region, 0, sizeof(struct shm_region_s));
  }

  /* Initialize shm free memory mgt */
  lk_memset(&g_shminfo->shm_free_mem_mgt, 0, sizeof(MM));
}

