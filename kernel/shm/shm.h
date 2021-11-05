// Copyright (C) 2021 Portions Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: BSD-3-Clause

/****************************************************************************
 * mm/shm/shm.h
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

#ifndef __MM_SHM_SHM_H
#define __MM_SHM_SHM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/lock.h>

#include "shm_mm.h"

//#define DEBUG

#define	CONFIG_ARCH_SHM_MAXREGIONS	1024	
#define	CONFIG_ARCH_SHM_SR_DATA_SIZE	(0x1000 * 24)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Bit definitions for the struct shm_region_s sr_flags field */

#define SRFLAG_AVAILABLE 0        /* Available if no flag bits set */
#define SRFLAG_INUSE     (1 << 0) /* Bit 0: Region is in use */
#define SRFLAG_UNLINKED  (1 << 1) /* Bit 1: Region perists while references */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure represents the state of one shared memory region
 * allocation.  Cast compatible with struct shmid_ds.
 */

struct shm_region_s
{
  struct shmid_ds sr_ds; /* Region info */
  BOOL  sr_flags;        /* See SRFLAGS_* definitions */
  key_t sr_key;          /* Lookup key */
  spinlock_t sr_lock;    /* Manages exclusive access to this region */

  /* pointer to IPC shared memory */
  unsigned char *sr_data; // get from shm_alloc(), page alligned
};

/* This structure represents the set of all shared memory regions.
 * Access to the region 
 */

struct shm_info_s
{
  unsigned long padding;
  spinlock_t si_lock; 	/* Manages exclusive access to this region */
  unsigned long padding2;
  int shm_init_flag;
  MM shm_free_mem_mgt;
  struct shm_region_s si_region[CONFIG_ARCH_SHM_MAXREGIONS];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* State of the all shared memory */

extern struct shm_info_s *g_shminfo;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: shm_destroy
 *
 * Description:
 *   Destroy a memory region.  This function is called:
 *
 *   - On certain conditions when shmget() is not successful in instantiating
 *     the full memory region and we need to clean up and free a table entry.
 *   - When shmctl() is called with cmd == IPC_RMID and there are no
 *     processes attached to the memory region.
 *   - When shmdt() is called after the last process detaches from memory
 *     region after it was previously marked for deletion by shmctl().
 *
 * Input Parameters:
 *   shmid - Shared memory identifier
 *
 * Returned Value:
 *   None
 *
 * Assumption:
 *   The caller holds either the region table semaphore or else the
 *   semaphore on the particular entry being deleted.
 *
 ****************************************************************************/

void shm_destroy(int shmid);
void shm_initialize(void);

int shmget(key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);

#endif /* __MM_SHM_SHM_H */
