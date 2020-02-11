#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "console_mmap.h"
#include "console_memory_config.h"

// all unikernels memory start address
unsigned long g_mmap_unikernel_mem_base_pa = 0;
unsigned long g_mmap_unikernel_mem_base_va = 0;

unsigned long g_mmap_shared_mem_base_pa = 0;
unsigned long g_mmap_shared_mem_base_va = 0;

unsigned long g_console_channels_info_va = 0;


/**
 * @brief munmap_channel()
 * @param console_channel channel
 * @param n_console_channels channel number
 * @return success (0), fail (-1)
 */
int munmap_console_channel(void *addr, unsigned long length)
{
  int err = 0;

  if(addr != NULL) {
    if(munmap(addr, (size_t) length) < 0) {
      err++;
    }
  }

  if(err)
    return -1;

  return 0;
}


/**
 * @brief mmap channels
 * @param console_channels console channels
 * @param n_console_channels the number of channels
 * @param opages  whole page number of out channels
 * @param ipages  whole page number of in channels
 * @return success (1), fail (0)
 */
int mmap_console_channel(channel_t *console_channels, int start_index, int n_console_channels, int opages, int ipages)
{
  int console_channels_offset = 0;

  int console_fd = 0;

  unsigned long in_cq_base = 0;
  unsigned long in_cq_base_pa = 0;
  unsigned long in_cq_base_pa_len = 0;

  unsigned long out_cq_base = 0;
  unsigned long out_cq_base_pa = 0;
  unsigned long out_cq_base_pa_len = 0;

  unsigned long console_channels_info_va = 0;
  unsigned long *console_channels_info = NULL;

  //int n_nodes = 0;

  console_fd = open("/dev/offload", O_RDWR) ;
  if (console_fd < 0) {
    printf("/dev/offload open error\n") ;

    return 0;
  }

#ifdef DEBUG
  printf("Console CONSOLE_CHANNEL_INFO_PA %lx \n", CONSOLE_CHANNEL_INFO_PA);
#endif
  console_channels_info_va = (unsigned long) mmap(NULL, (size_t) PAGE_SIZE_4K, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, (unsigned long) CONSOLE_CHANNEL_INFO_PA);
  g_console_channels_info_va = (unsigned long) console_channels_info_va;

  console_channels_info = (unsigned long *) console_channels_info_va;

  for(console_channels_offset = start_index; console_channels_offset < start_index+1; console_channels_offset++) {
    out_cq_base_pa = (unsigned long) CONSOLE_CHANNEL_BASE_PA + (unsigned long) console_channels_offset * (unsigned long) (opages + ipages) * (unsigned long) PAGE_SIZE_4K;
    out_cq_base_pa_len = (unsigned long) (opages * PAGE_SIZE_4K);
    out_cq_base = (unsigned long) mmap(NULL, out_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, out_cq_base_pa);

    if(out_cq_base == (unsigned long) MAP_FAILED ) {
      printf("mmap failed.\n") ;
      close(console_fd) ;

      return 0;
    }

    console_channels[console_channels_offset].out_cq = (struct circular_queue *)(out_cq_base);
    console_channels[console_channels_offset].out_cq_len = out_cq_base_pa_len;

    // cq_init() will be done in unikernel
    // cq_init(console_channels[console_channels_offset].out_cq, (opages - 1) / CQ_ELE_PAGE_NUM);

    in_cq_base_pa = (unsigned long) out_cq_base_pa + (opages * PAGE_SIZE_4K);
    in_cq_base_pa_len = (unsigned long) (ipages * PAGE_SIZE_4K);
    in_cq_base = (unsigned long) mmap(NULL, in_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, in_cq_base_pa);

    if(in_cq_base == (unsigned long) MAP_FAILED ) {
      printf("mmap failed.\n") ;
      munmap(console_channels[console_channels_offset].out_cq, console_channels[console_channels_offset].out_cq_len);
      close(console_fd) ;

      return 0;
    }

    console_channels[console_channels_offset].in_cq = (struct circular_queue *)(in_cq_base);
    console_channels[console_channels_offset].in_cq_len = in_cq_base_pa_len;

    // cq_init() will be done in unikernel
    // cq_init(console_channels[console_channels_offset].in_cq, (ipages - 1) / CQ_ELE_PAGE_NUM);
  }
  console_channels_info = ((unsigned long *) console_channels_info_va) + start_index;
  *(console_channels_info) = (unsigned long) (CONSOLE_MAGIC + start_index);

  close(console_fd);

  return 1;
}


/**
 * @brief mmap unikernels' memory
 * @return success (0), fail (1)
 */
int mmap_unikernel_memory(int start_index)
{
  unsigned long unikernel_mem_base_pa_len;
  unsigned long shared_mem_base_pa_len;

  int console_fd = 0;

  console_fd = open("/dev/offload", O_RDWR | O_SYNC) ;
  if ( console_fd < 0 ) {
    printf("/dev/offload open error\n") ;

    return 1;
  }

  g_mmap_unikernel_mem_base_pa = (unsigned long) UNIKERNELS_MEM_BASE_PA + (((unsigned long) (MEMORYS_PER_NODE * start_index)) << 30);
  unikernel_mem_base_pa_len = ((unsigned long) MEMORYS_PER_NODE) << 30;

  g_mmap_unikernel_mem_base_va = (unsigned long) mmap(NULL, unikernel_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, g_mmap_unikernel_mem_base_pa);

  if ( g_mmap_unikernel_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(console_fd) ;

    return 1;
  }

  g_mmap_shared_mem_base_pa = (unsigned long) SHARED_MEM_BASE_PA;
  shared_mem_base_pa_len = (unsigned long) SHARED_MEMORY_SIZE << 30;

  g_mmap_shared_mem_base_va = (unsigned long) mmap(NULL, shared_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, g_mmap_shared_mem_base_pa);

  if ( g_mmap_shared_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(console_fd) ;

    return 1;
  }

  close(console_fd) ;

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

  if(pa > g_mmap_unikernel_mem_base_pa) {
    offset = (unsigned long) (pa - g_mmap_unikernel_mem_base_pa);
    return (g_mmap_unikernel_mem_base_va + offset);
  }
  else {
    offset = (unsigned long) (pa - g_mmap_shared_mem_base_pa);
    return (g_mmap_shared_mem_base_va + offset);
  }

}


/**
 * @brief get unikernels' start physical address
 * @return (start physical address)
 */
unsigned long get_pa_base() 
{
  return (g_mmap_unikernel_mem_base_pa);
}


/**
 * @brief get driver's start virutal address that matches start physical address of unikernel
 * @return (start virtual address)
 */
unsigned long get_va_base() 
{
  return (g_mmap_unikernel_mem_base_va);
}


