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


/**
 * @brief munmap_channels()
 * @param console_channels channel
 * @param n_console_channels channel number
 * @return success (0), fail (-1)
 */
int munmap_console_channels(channel_t *console_channels, int n_console_channels)
{
  int console_channels_offset = 0;
  int err = 0;

  // munmap channel
  for(console_channels_offset = 0; console_channels_offset < n_console_channels; console_channels_offset++) {
    if(munmap(console_channels[console_channels_offset].out_cq, console_channels[console_channels_offset].out_cq_len) < 0) {
      printf("munmap failed.\n");
      err++;
    }
    if(munmap(console_channels[console_channels_offset].in_cq, console_channels[console_channels_offset].in_cq_len) < 0) {
      printf("munmap failed.\n");
      err++;
    }
  }

  if(err)
    return -1;

  return 0;
}

/**
 * @brief mmap channels
 * @param console_channels offload channels
 * @param n_console_channels the number of channels
 * @param opages  whole page number of out channels
 * @param ipages  whole page number of in channels
 * @return success (1), fail (0)
 */
int mmap_console_channels(channel_t *console_channel, int node_id, int n_console_channels, int opages, int ipages)
{
  int offload_fd = 0;

  unsigned long in_cq_base = 0;
  unsigned long in_cq_base_pa = 0;
  unsigned long in_cq_base_pa_len = 0;

  unsigned long out_cq_base = 0;
  unsigned long out_cq_base_pa = 0;
  unsigned long out_cq_base_pa_len = 0;

  offload_fd = open("/dev/offload", O_RDWR) ;
  if (offload_fd < 0) {
    printf("/dev/offload open error\n") ;

    return 0;
  }
  //printf("node id: %d, #ch %d, #opages %d, #ipages %d", node_id, n_console_channels, opages, ipages) ;

    // mmap ocq of ith channel
  out_cq_base_pa = ((unsigned long) (CONSOLE_CHANNEL_BASE_PA)) + node_id * (opages + ipages) * PAGE_SIZE_4K;
  out_cq_base_pa_len = (unsigned long) (opages * PAGE_SIZE_4K);
  out_cq_base = (unsigned long) mmap(NULL, out_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, out_cq_base_pa);

  if(out_cq_base == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    munmap_console_channels(console_channel, 1);
    close(offload_fd) ;
    return 0;
  }

  console_channel->out_cq = (struct circular_queue *)(out_cq_base);
  console_channel->out_cq_len = out_cq_base_pa_len;

  //init cq
  //cq_init(console_channel.out_cq, (opages - 1) / CQ_ELE_PAGE_NUM);
  //mutex_init(console_channel.out_cq->lock);

  // mmap icq of ith channel
  in_cq_base_pa = (unsigned long) out_cq_base_pa + (opages * PAGE_SIZE_4K);
  in_cq_base_pa_len = (unsigned long) (ipages * PAGE_SIZE_4K);
  in_cq_base = (unsigned long) mmap(NULL, in_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, in_cq_base_pa);

  if(in_cq_base == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    munmap(console_channel->out_cq, console_channel->out_cq_len);
    munmap_console_channels(console_channel, 1);
    close(offload_fd) ;

    return 0;
  }

  console_channel->in_cq = (struct circular_queue *)(in_cq_base);
  console_channel->in_cq_len = in_cq_base_pa_len;

  //init cq
  //cq_init(console_channel.in_cq, (ipages - 1) / CQ_ELE_PAGE_NUM);
  //mutex_init(console_channel.in_cq->lock);
  //printf("\n icq - ocq distance %lx ", in_cq_base_pa - out_cq_base_pa) ;

  close(offload_fd);

  return 1;
}


/**
 * @brief mmap unikernels' whole memory
 * @return success (0), fail (1)
 */
int mmap_unikernel_memory(int node_id)
{
  unsigned long unikernel_mem_base_pa_len;

  int offload_fd = 0;

  offload_fd = open("/dev/offload", O_RDWR | O_SYNC) ;
  if ( offload_fd < 0 ) {
    printf("/dev/offload open error\n") ;

    return 1;
  }

  g_mmap_unikernel_mem_base_pa = (unsigned long) (UNIKERNELS_MEM_BASE_PA + MEMORYS_PER_NODE * node_id * 1024UL*1024UL*1024UL);
  unikernel_mem_base_pa_len = (unsigned long) (MEMORYS_PER_NODE * 1024UL*1024UL*1024UL);

  g_mmap_unikernel_mem_base_va = (unsigned long) mmap(NULL, unikernel_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, g_mmap_unikernel_mem_base_pa);

  if ( g_mmap_unikernel_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(offload_fd) ;

    return 1;
  }

  //printf("mmap = virtual address: 0x%lx physical address begin: 0x%lx end: 0x%lx\n", g_mmap_unikernels_mem_base_va, kernels_mem_base_pa, kernels_mem_base_pa + kernels_mem_base_pa_len) ;

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

  //offset = (unsigned long) pa - UNIKERNELS_MEM_BASE_PA;
  offset = (unsigned long) (pa - g_mmap_unikernel_mem_base_pa);

  return (g_mmap_unikernel_mem_base_va + offset);
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


