#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "offload_mmap.h"
#include "offload_memory_config.h"

// all unikernels memory start address
unsigned long g_mmap_unikernels_mem_base_va = 0;

// offload channel info address
unsigned long g_offload_channels_info_va = 0;

#ifdef LOCK_ENABLED
unsigned long g_offload_locks_va = 0;
#endif

/**
 * @brief munmap_channels()
 * @param offload_channels channel
 * @param n_offload_channels channel number
 * @return success (0), fail (-1)
 */
int munmap_channels(channel_t *offload_channels, int n_offload_channels)
{
  int offload_channels_offset = 0;
  int err = 0;

  // munmap channel
  for(offload_channels_offset = 0; offload_channels_offset < n_offload_channels; offload_channels_offset++) {
    if(munmap(offload_channels[offload_channels_offset].out_cq, offload_channels[offload_channels_offset].out_cq_len) < 0) {
      printf("munmap failed.\n");
      err++;
    }
    if(munmap(offload_channels[offload_channels_offset].in_cq, offload_channels[offload_channels_offset].in_cq_len) < 0) {
      printf("munmap failed.\n");
      err++;
    }
  }

  // munmap channel informaiton
  if(munmap((void *) g_offload_channels_info_va, (size_t) PAGE_SIZE_4K) < 0) {
    printf("munmap failed.\n");
    err++;
  }

#ifdef LOCK_ENABLED
  if(g_offload_locks_va != 0) {
    if(munmap((void *) g_offload_locks_va, (size_t) PAGE_SIZE_4K*2) < 0) {
      printf("munmap failed.\n");
      err++;
    }
  }
#endif

  if(err)
    return -1;

	return 0;
}

/**
 * @brief mmap channels
 * @param offload_channels offload channels
 * @param n_offload_channels the number of channels
 * @param opages  whole page number of out channels
 * @param ipages  whole page number of in channels
 * @return success (1), fail (0)
 */
int mmap_channels(channel_t *offload_channels, int n_offload_channels, int opages, int ipages)
{
  int offload_channels_offset = 0;

  int offload_fd = 0;

  unsigned long in_cq_base = 0;
  unsigned long in_cq_base_pa = 0;
  unsigned long in_cq_base_pa_len = 0;

  unsigned long out_cq_base = 0;
  unsigned long out_cq_base_pa = 0;
  unsigned long out_cq_base_pa_len = 0;

  unsigned long offload_channels_info_va = 0;
  //unsigned long offload_channels_info_pa = 0;
  unsigned long *offload_channels_info = NULL;

#ifdef LOCK_ENABLED
  az_spinlock_t *locks = NULL;
  int locks_count = 0;
#endif

  offload_fd = open("/dev/offload", O_RDWR) ;
  if (offload_fd < 0) {
    printf("/dev/offload open error\n") ;

    return 0;
  }

  offload_channels_info_va = (unsigned long) mmap(NULL, (size_t) PAGE_SIZE_4K, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, (off_t) OFFLOAD_CHANNEL_INFO_PA);
  g_offload_channels_info_va = (unsigned long) offload_channels_info_va;

  // set channel information
  offload_channels_info = (unsigned long *) offload_channels_info_va;
  *(offload_channels_info++) = (unsigned long) 0;
  *(offload_channels_info++) = (unsigned long) n_offload_channels;
  *(offload_channels_info++) = (unsigned long) opages;
  *(offload_channels_info) = (unsigned long) ipages;

  for(offload_channels_offset = 0; offload_channels_offset < n_offload_channels; offload_channels_offset++) {
    // mmap ocq of ith channel
    out_cq_base_pa = (unsigned long) OFFLOAD_CHANNEL_BASE_PA + offload_channels_offset * (opages + ipages) * PAGE_SIZE_4K;
    out_cq_base_pa_len = (unsigned long) (opages * PAGE_SIZE_4K);
    out_cq_base = (unsigned long) mmap(NULL, out_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, out_cq_base_pa);

    if(out_cq_base == (unsigned long) MAP_FAILED ) {
      printf("mmap failed.\n") ;
      munmap_channels(offload_channels, offload_channels_offset);
      close(offload_fd) ;

      return 0;
    }

    offload_channels[offload_channels_offset].out_cq = (struct circular_queue *)(out_cq_base);
    offload_channels[offload_channels_offset].out_cq_len = out_cq_base_pa_len;

    //init cq
    cq_init(offload_channels[offload_channels_offset].out_cq, (opages - 1) / CQ_ELE_PAGE_NUM);

    // mmap icq of ith channel
    in_cq_base_pa = (unsigned long) out_cq_base_pa + (opages * PAGE_SIZE_4K);
    in_cq_base_pa_len = (unsigned long) (ipages * PAGE_SIZE_4K);
    in_cq_base = (unsigned long) mmap(NULL, in_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, in_cq_base_pa);

    if(in_cq_base == (unsigned long) MAP_FAILED ) {
      printf("mmap failed.\n") ;
      munmap(offload_channels[offload_channels_offset].out_cq, offload_channels[offload_channels_offset].out_cq_len);
      munmap_channels(offload_channels, offload_channels_offset);
      close(offload_fd) ;
 
      return 0;
    }

    offload_channels[offload_channels_offset].in_cq = (struct circular_queue *)(in_cq_base);
    offload_channels[offload_channels_offset].in_cq_len = in_cq_base_pa_len;

    //init cq
    cq_init(offload_channels[offload_channels_offset].in_cq, (ipages - 1) / CQ_ELE_PAGE_NUM);
  }

#ifdef LOCK_ENABLED
  g_offload_locks_va = (unsigned long) mmap(NULL, PAGE_SIZE_4K * 2, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, OFFLOAD_CHANNEL_LOCK_PA);
  locks = (az_spinlock_t *) g_offload_locks_va;

  for(offload_channels_offset = 0, locks_count = 0; (offload_channels_offset < n_offload_channels) && (offload_channels_offset < LOCK_ENABLED_MAX_CHANNELS_NUM); offload_channels_offset++ ) {
    az_spinlock_init(locks + locks_count++ * 64);
    az_spinlock_init(locks + locks_count++ * 64);
  }
#endif

  //set OFFLOAD_MAGIC
  offload_channels_info = (unsigned long *) offload_channels_info_va;
  *(offload_channels_info) = (unsigned long) OFFLOAD_MAGIC;

  close(offload_fd);

  return 1;
}


/**
 * @brief mmap unikernels' memory
 * @return success (0), fail (1)
 */
int mmap_unikernels_memory()
{
  unsigned long kernels_mem_base_pa;
  unsigned long kernels_mem_base_pa_len;

  int offload_fd = 0;

  offload_fd = open("/dev/offload", O_RDWR | O_SYNC) ;
  if ( offload_fd < 0 ) {
    printf("/dev/offload open error\n") ;

    return 1;
  }

  kernels_mem_base_pa = UNIKERNELS_MEM_BASE_PA; // 48G
  kernels_mem_base_pa_len = UNIKERNELS_MEM_SIZE; // 48G * 3

  g_mmap_unikernels_mem_base_va = (unsigned long) mmap(NULL, kernels_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, kernels_mem_base_pa);

  if ( g_mmap_unikernels_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(offload_fd) ;

    return 1;
  }

  printf("mmap = virtual address: 0x%lx physical address begin: 0x%lx end: 0x%lx\n", g_mmap_unikernels_mem_base_va, kernels_mem_base_pa, kernels_mem_base_pa + kernels_mem_base_pa_len) ;

  close(offload_fd) ;

  return 0;
}

/**
 * @brief mmapped virtual address that matches physical address of unikernel
 * @param pa physical address of unikernel
 * @return (virual address)
 */
unsigned long get_va(unsigned long pa)
{
  unsigned long offset = 0;

  if(pa == 0)
    return 0;

  offset = (unsigned long) pa - UNIKERNELS_MEM_BASE_PA;

  return (g_mmap_unikernels_mem_base_va + offset);
}


/**
 * @brief get unikernels' start physical address
 * @return (start physical address)
 */
unsigned long get_pa_base() 
{
  return (UNIKERNELS_MEM_BASE_PA);
}

/**
 * @brief get driver's start virutal address that matches start physical address of unikernel
 * @return (start virtual address)
 */
unsigned long get_va_base() 
{
  return (g_mmap_unikernels_mem_base_va);
}


