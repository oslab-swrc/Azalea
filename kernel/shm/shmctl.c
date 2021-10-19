// Copyright (C) 2014 Gregory Nutt. All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause

/****************************************************************************
 * mm/shm/shmctl.c
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
#include <errno.h>
#include <sys/lock.h>
#include <sys/shm.h>

#include "console.h"
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: shmctl
 *
 * Description:
 *   The shmctl() function provides a variety of shared memory control
 *   operations as specified by cmd. The following values for cmd are
 *   available:
 *
 *     - IPC_STAT
 *       Place the current value of each member of the shmid_ds data
 *       structure associated with shmid into the structure pointed to by
 *       buf.
 *     - IPC_SET
 *       Set the value of the shm_perm.mode member of the shmid_ds data
 *       structure associated with shmid to the corresponding value found
 *       in the structure pointed to by buf.
 *     - IPC_RMID
 *       Remove the shared memory identifier specified by shmid from the
 *       system and destroy the shared memory segment and shmid_ds data
 *       structure associated with it.
 *
 * Input Parameters:
 *   shmid - Shared memory identifier
 *   cmd - shmctl() command
 *   buf - Data associated with the shmctl() command
 *
 * Returned Value:
 *   Upon successful completion, shmctl() will return 0; otherwise, it will
 *   return -1 and set errno to indicate the error.
 *
 *     - EACCES
 *       The argument cmd is equal to IPC_STAT and the calling process does
 *       not have read permission.
 *     - EINVAL
 *       The value of shmid is not a valid shared memory identifier, or the
 *       value of cmd is not a valid command.
 *     - EPERM
 *       The argument cmd is equal to IPC_RMID or IPC_SET and the effective
 *       user ID of the calling process is not equal to that of a process
 *       with appropriate privileges and it is not equal to the value of
 *       shm_perm.cuid or shm_perm.uid in the data structure associated with
 *       shmid.
 *     - EOVERFLOW
 *       The cmd argument is IPC_STAT and the gid or uid value is too large
 *       to be stored in the structure pointed to by the buf argument.
 *
 * POSIX Deviations:
 *     - IPC_SET.  Does not set the the shm_perm.uid or shm_perm.gid
 *       members of the shmid_ds data structure associated with shmid
 *       because user and group IDs are not yet supported by NuttX
 *     - IPC_SET.  Does not restrict the operation to processes with
 *       appropriate privileges or matching user IDs in shmid_ds data
 *       structure associated with shmid.  Again because user IDs and
 *       user/group privileges are are not yet supported by NuttX
 *     - IPC_RMID.  Does not restrict the operation to processes with
 *       appropriate privileges or matching user IDs in shmid_ds data
 *       structure associated with shmid.  Again because user IDs and
 *       user/group privileges are are not yet supported by NuttX
 *
 ****************************************************************************/

int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
  struct shm_region_s *region;

  g_shminfo = (struct shm_info_s *) SHM_INFO_S_VA;

  //DEBUGASSERT(shmid >= 0 && shmid < CONFIG_ARCH_SHM_MAXREGIONS);
  if(shmid < 0 || shmid >= CONFIG_ARCH_SHM_MAXREGIONS) {
    lk_print("shmctl() - Invalid shmid %d\n", shmid);
    return (-1);
  }

  /* Get the region associated with the shmid */
  region =  &g_shminfo->si_region[shmid];

  //DEBUGASSERT((region->sr_flags & SRFLAG_INUSE) != 0);
  if((region->sr_flags & SRFLAG_INUSE) == 0) {
    lk_print("shmctl() - Invalid shmid %d\n", shmid);
    return (-1);
  }

  /* Get exclusive access to the region data structure */
  spinlock_lock(&region->sr_lock);

  /* Handle the request according to the received cmd */
  switch (cmd)
  {
    case IPC_STAT:
    {
      /* Place the current value of each member of the shmid_ds data
       * structure associated with shmid into the structure pointed to
       * by buf.
       */

      //DEBUGASSERT(buf);
      if(buf == NULL) {
        goto errout_with_semaphore;
      }
      lk_memcpy(buf, &region->sr_ds, sizeof(struct shmid_ds));
    }
    break;

    case IPC_SET:
    {
      /* Set the value of the shm_perm.mode member of the shmid_ds
       * data structure associated with shmid to the corresponding
       * value found in the structure pointed to by buf.
       */

      region->sr_ds.shm_perm.mode = buf->shm_perm.mode;
    }
    break;

    case IPC_RMID:
    {
      /* Are any processes attached to the region? */

      if (region->sr_ds.shm_nattch > 0)
      {
        /* Yes.. just set the UNLINKED flag.  The region will be removed when there are no longer any processes attached to it.
               */

        region->sr_flags |= SRFLAG_UNLINKED;
      }
      else
      {
        /* No.. free the entry now */

        shm_destroy(shmid);

        /* Don't try anything further on the deleted region */

        //return OK;
        return (0);
      }
    }
    break;

    default:
      lk_print("shmctl() - Unrecognized command: %d\n", cmd);
      //ret = -EINVAL;
      goto errout_with_semaphore;
  }

  /* Save the process ID of the the last operation */
  region = &g_shminfo->si_region[shmid];
  region->sr_ds.shm_lpid = sys_getpid();

  /* Save the time of the last shmctl() */
  //TODO region->sr_ds.shm_ctime = time(NULL);

  /* Release our lock on the entry */
  spinlock_unlock(&region->sr_lock);

  //return OK;
  return (0);

errout_with_semaphore:

  /* Release our lock on the entry */
  spinlock_unlock(&region->sr_lock);

  //return ERROR;
  return (-1);
}

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

void shm_destroy(int shmid)
{

  struct shm_region_s *region;

  g_shminfo = (struct shm_info_s *) SHM_INFO_S_VA;

  region =  &g_shminfo->si_region[shmid];

  // free shred memory
  if(region->sr_data != NULL)
    shm_free(region->sr_data);

  /* Reset the region entry to its initial state */
  lk_memset(region, 0, sizeof(struct shm_region_s));
}

