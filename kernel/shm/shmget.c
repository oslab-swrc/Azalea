// Copyright (C) 2021 Portions Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: BSD-3-Clause

/****************************************************************************
 * mm/shm/shmget.c
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
#include "console.h"
#include "console_function.h"
#include "shm.h"
#include "shm_mm_config.h"
#include "thread.h"
#include "utility.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: shm_find
 *
 * Description:
 *   Find the shared memory region with matching key
 *
 * Input parameters:
 *   key - The value that uniquely identifies a shared memory region.
 *
 * Returned value:
 *   On success, an index in the range of 0 to CONFIG_ARCH_SHM_MAXREGIONS-1
 *   is returned to identify the matching region; -ENOENT is returned on
 *   failure.
 *
 ****************************************************************************/

static int shm_find(key_t key)
{
  int i;

#ifdef DEBUG
  cs_printf("shm_find(): key = %d\n", key);
#endif
  for (i = 0; i < CONFIG_ARCH_SHM_MAXREGIONS; i++)
  {
     if (g_shminfo->si_region[i].sr_key == key)
     {
        return i;
     }
  }

  return (-1);
}

/****************************************************************************
 * Name: shm_reserve
 *
 * Description:
 *   Allocate an unused shared memory region.  That is one with a key of -1
 *
 * Input parameters:
 *   None
 *
 * Returned value:
 *   On success, an index in the range of 0 to CONFIG_ARCH_SHM_MAXREGIONS-1
 *   is returned to identify the matching region; -ENOSPC is returned on
 *   failure.
 *
 ****************************************************************************/

static int shm_reserve(key_t key, int shmflg)
{
  struct shm_region_s *region;
  int i;

#ifdef DEBUG
  cs_printf("shm_reserve(): key = %d, shmflg = %d\n", key, shmflg);
#endif
  for (i = 0; i < CONFIG_ARCH_SHM_MAXREGIONS; i++)
    {
      /* Is this region in use? */

      region = &g_shminfo->si_region[i];
      if (region->sr_flags == SRFLAG_AVAILABLE)
      {
        /* No... reserve it for the caller now */

         //lk_memset(region, 0, sizeof(struct shm_region_s));
         region->sr_key   = key;
         region->sr_flags = SRFLAG_INUSE;
 
         spinlock_init(&region->sr_lock);
 
         /* Set the low-order nine bits of shm_perm.mode to the low-order
          * nine bits of shmflg.
          */
 
         region->sr_ds.shm_perm.mode = shmflg & IPC_MODE;
 
         /* The value of shm_segsz is left equal to zero for now because no
          * memory has yet been allocated.
          *
          * The values of shm_lpid, shm_nattch, shm_atime, and shm_dtime are
          * set equal to 0.
          */
 
         /* The value of shm_ctime is set equal to the current time. */
 
         //TODO: region->sr_ds.shm_ctime = time(NULL);
         return i;
      }
    }

  return -ENOSPC;
}

/****************************************************************************
 * Name: shm_extend
 *
 * Description:
 *   Extend the size of a memory regions by allocating physical pages as
 *   necessary
 *
 * Input parameters:
 *   shmid - The index of the region of interest in the shared memory region
 *     table.
 *   size - The new size of the region.
 *
 * Returned value:
 *   Zero is returned on success; -ENOMEM is returned on failure.
 *   (Should a different error be returned if the region is just too big?)
 *
 ****************************************************************************/

int shm_extend(int shmid, size_t size)
{
  struct shm_region_s *region =  NULL;

#ifdef DEBUG
  cs_printf("shm_extend(): shmid = %d, size = %d\n", shmid, size);
#endif

  region =  &g_shminfo->si_region[shmid];

  /* Set the new region size and return success */

  if(size > region->sr_ds.shm_segsz) {
    // check page count 
    size_t allocated_block_size = shm_get_buddy_block_size(region->sr_ds.shm_segsz); 
    if(size > allocated_block_size) {
    //if((int) (size / PAGE_MIN_SIZE) != (int) (region->sr_ds.shm_segsz / PAGE_MIN_SIZE)) {
      return -ENOMEM;
    }
    else {
      lk_memset(&(region->sr_data[region->sr_ds.shm_segsz]), 0, (size - region->sr_ds.shm_segsz));
      region->sr_ds.shm_segsz = size;
    }
  }

  //return OK;
  return (1);
}

/****************************************************************************
 * Name: shm_create
 *
 * Description:
 *   Create the shared memory region.
 *
 * Input parameters:
 *   key     - The key that is used to access the unique shared memory
 *             identifier.
 *   size    - The shared memory region that is created will be at least
 *             this size in bytes.
 *   shmflgs - See IPC_* definitions in sys/ipc.h.  Only the values
 *             IPC_PRIVATE or IPC_CREAT are supported.
 *
 * Returned value:
 *   Zero is returned on success;  A negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

static int shm_create(key_t key, size_t size, int shmflg)
{
  struct shm_region_s *region = NULL;
  int shmid = -1;
  int ret = -1;

#ifdef DEBUG
  cs_printf("shm_created(): key = %d, size = %d, shmflg = %d\n", key, size, shmflg);
#endif

  /* Reserve the shared memory region */
  ret = shm_reserve(key, shmflg);

  if (ret < 0)
  {
    lk_print("shm_create(): shm_reserve() failed: %d\n", ret);
    return ret;
  }

  /* Save the shared memory ID */
  shmid = ret;

  /* Then allocate the physical memory (by extending it from the initial
   * size of zero).
   */

  region = &g_shminfo->si_region[shmid];

  region->sr_data = (unsigned char *) shm_alloc(size);
#ifdef DEBUG
  cs_printf("shm_create(): region->sr_data addr = %q\n", region->sr_data);
#endif

  if(region->sr_data == NULL) {
    lk_print("shm_create(): shm memory allocation failed.\n");

    shm_destroy(shmid);
    return -ENOMEM;
  }

  /* Save the size of the request memory size */
  region->sr_ds.shm_segsz = size;

  /* Save the process ID of the creator */
  region->sr_ds.shm_cpid = sys_getpid();

  /* Return the shared memory ID */

  return shmid;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: shmget
 *
 * Description:
 *   The shmget() function will return the shared memory identifier
 *   associated with key.
 *
 *   A shared memory identifier, associated data structure, and shared
 *   memory segment of at least size bytes is created for key if one of the
 *   following is true:
 *
 *     - The argument key is equal to IPC_PRIVATE.
 *     - The argument key does not already have a shared memory identifier
 *       associated with it and (shmflg & IPC_CREAT) is non-zero.
 *
 *   Upon creation, the data structure associated with the new shared memory
 *   identifier will be initialized as follows:
 *
 *     - The low-order nine bits of shm_perm.mode are set equal to the low-
 *       order nine bits of shmflg.
 *     - The value of shm_segsz is set equal to the value of size.
 *     - The values of shm_lpid, shm_nattch, shm_atime, and shm_dtime are
 *       set equal to 0.
 *     - The value of shm_ctime is set equal to the current time.
 *
 *   When the shared memory segment is created, it will be initialized with
 *   all zero values.
 *
 * Input Parameters:
 *   key     - The key that is used to access the unique shared memory
 *             identifier.
 *   size    - The shared memory region that is created will be at least
 *             this size in bytes.
 *   shmflgs - See IPC_* definitions in sys/ipc.h.  Only the values
 *             IPC_PRIVATE or IPC_CREAT are supported.
 *
 * Returned Value:
 *   Upon successful completion, shmget() will return a non-negative
 *   integer, namely a shared memory identifier; otherwise, it will return
 *   -1 and set errno to indicate the error.
 *
 *     - EACCES
 *       A shared memory identifier exists for key but operation permission
 *       as specified by the low-order nine bits of shmflg would not be
 *       granted.
 *     - EEXIST
 *       A shared memory identifier exists for the argument key but
 *      (shmflg & IPC_CREAT) && (shmflg & IPC_EXCL) are non-zero.
 *     - EINVAL
 *       A shared memory segment is to be created and the value of size is
 *       less than the system-imposed minimum or greater than the system-
 *       imposed maximum.
 *     - EINVAL
 *       No shared memory segment is to be created and a shared memory
 *       segment exists for key but the size of the segment associated with
 *       it is less than size and size is not 0.
 *     - ENOENT
 *       A shared memory identifier does not exist for the argument key and
 *       (shmflg & IPC_CREAT) is 0.
 *     - ENOMEM
 *       A shared memory identifier and associated shared memory segment
 *       will be created, but the amount of available physical memory is
 *       not sufficient to fill the request.
 *     - ENOSPC
 *       A shared memory identifier is to be created, but the system-imposed
 *       limit on the maximum number of allowed shared memory identifiers
 *       system-wide would be exceeded.
 *
 * POSIX Deviations:
 *     - The values of shm_perm.cuid, shm_perm.uid, shm_perm.cgid, and
 *       shm_perm.gid should be set equal to the effective user ID and
 *       effective group ID, respectively, of the calling process.
 *       The NuttX ipc_perm structure, however, does not support these
 *       fields because user and group IDs are not yet supported by NuttX.
 *
 ****************************************************************************/

int shmget(key_t key, size_t size, int shmflg)
{
  struct shm_region_s *region;
  int shmid = -1;
  int ret;

  /* Check for the special case where the caller doesn't really want shared
   * memory (they why do they bother to call us?)
   */
  
  g_shminfo = (struct shm_info_s *) SHM_INFO_S_VA;

#ifdef DEBUG
  cs_printf("shmget(): key = %d, size = %d, shmflg = %d\n", key, size, shmflg);
  cs_printf("shmget(): shm_info addr = %q\n", g_shminfo);
#endif

  if (key == IPC_PRIVATE)
  {
    /* Not yet implemented */

    ret = -ENOSYS;
    goto errout;
  }


  /* Get exclusive access to the global list of shared memory regions */
#ifdef DEBUG
  cs_printf("shmget(): try lock\n");
#endif
  spinlock_lock(&g_shminfo->si_lock);
#ifdef DEBUG
  cs_printf("shmget(): lock success\n");
#endif

  if(g_shminfo->shm_init_flag == 0) {
    shm_initialize();
    shm_free_mem_init();
    g_shminfo->shm_init_flag = 1;
  }

  /* Find the requested memory region */
  ret = shm_find(key);

  if (ret < 0)
  {
    /* The memory region does not exist.. create it if IPC_CREAT is
     * included in the shmflags.
     */

    if ((shmflg & IPC_CREAT) != 0)
    {
      /* Create the memory region */
      ret = shm_create(key, size, shmflg);
      if (ret < 0)
      {
        //shmdbg("shm_create failed: %d\n", ret);
        lk_print("shmget(): shm_create() failed: %d\n", ret);
        goto errout_with_lock;
      }

      /* Return the shared memory ID */
      shmid = ret;
    }
    else
    {
      /* Fail with ENOENT */
      goto errout_with_lock;
    }
  }
  /* The region exists */
  else
  {
    /* Remember the shared memory ID */

    shmid = ret;

    /* Is the region big enough for the request? */

    region = &g_shminfo->si_region[shmid];
    if (region->sr_ds.shm_segsz < size)
    {
      /* We we asked to create the region?  If so we can just
       * extend it.
       *
       * REVISIT: We should check the mode bits of the regions
       * first
       */

      if ((shmflg & IPC_CREAT) != 0)
      {
        /* Extend the region */
        ret = shm_extend(shmid, size);
        if (ret < 0)
        {
          lk_print("shmget(); shm_extend() failed: %d\n", ret);
          goto errout_with_lock;
        }
      }
      else
      {
        /* Fail with EINVAL */
        ret = -EINVAL;
        goto errout_with_lock;
      }
    }

    /* The region is already big enough or else we successfully
     * extended the size of the region.  If the region was previously
     * deleted, but waiting for processes to detach from the region,
     * then it is no longer deleted.
     */

    region->sr_flags = SRFLAG_INUSE;
  }

  /* Release our lock on the shared memory region list */
  spinlock_unlock(&g_shminfo->si_lock);

  return shmid;

errout_with_lock:
  /* Release our lock on the shared memory region list */
  spinlock_unlock(&g_shminfo->si_lock);

errout:
  return (-1);
}

