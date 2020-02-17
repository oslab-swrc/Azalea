#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "console_mmap.h"
#include "console_memory_config.h"

//#define	DEBUG

// all unikernels memory start address
unsigned long g_mmap_kernel_mem_base_va = 0;
unsigned long g_kernel_mem_base_pa = 0;

// offload channel address
unsigned long g_mmap_channel_mem_base_va = 0;
unsigned long g_channel_mem_base_pa = 0;

// offload channel info address
unsigned long g_mmap_console_channels_info_va = 0;
unsigned long g_mmap_console_channels_info_va_len = 0;


/**
 * @brief munmap_channel()
 * @param console_channel channel
 * @return success (1), fail (0)
 */
int munmap_console_channel(channel_t *console_channel)
{
  int err = 0;

  if(g_mmap_console_channels_info_va != 0)
    if (munmap((void *) g_mmap_console_channels_info_va, g_mmap_console_channels_info_va_len) < 0)
      err++;

  if(console_channel->out_cq != NULL)
    if (munmap(console_channel->out_cq, console_channel->out_cq_len) < 0)
      err++;

  if(console_channel->in_cq != NULL)
    if (munmap(console_channel->in_cq, console_channel->in_cq_len) < 0)
      err++;

  if(err)
    return (0);

  return (1);
}


/**
 * @brief mmap channels
 * @param console_channels console channels
 * @param opages  whole page number of out channels
 * @param ipages  whole page number of in channels
 * @return success (1), fail (0)
 */
int mmap_console_channel(channel_t *console_channel, int start_index, int opages, int ipages)
{
  int console_fd = 0;

  unsigned long in_cq_base = 0;
  unsigned long in_cq_base_pa = 0;
  unsigned long in_cq_base_pa_len = 0;

  unsigned long out_cq_base = 0;
  unsigned long out_cq_base_pa = 0;
  unsigned long out_cq_base_pa_len = 0;

  unsigned long *console_channels_info = NULL;

  console_fd = open("/dev/lk", O_RDWR) ;
  if (console_fd < 0) {
    printf("/dev/lk open error\n") ;

    return (0);
  }

  //////////////////////////////////////////////////////////////////////////////
  /* mmap console channels info                                               */
  //////////////////////////////////////////////////////////////////////////////
  g_mmap_console_channels_info_va_len = PAGE_SIZE_4K;
  g_mmap_console_channels_info_va = (unsigned long) mmap(NULL, (size_t) g_mmap_console_channels_info_va_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, (unsigned long) CONSOLE_CHANNEL_INFO_PA);

#ifdef DEBUG
  printf("console_channels_info_pa: %lx console_channels_info_va: %lx \n", (unsigned long) CONSOLE_CHANNEL_INFO_PA, (unsigned long) g_mmap_console_channels_info_va);
#endif

  //////////////////////////////////////////////////////////////////////////////
  /* mmap out cq & in cq                                                      */
  //////////////////////////////////////////////////////////////////////////////
  out_cq_base_pa = (unsigned long) CONSOLE_CHANNEL_BASE_PA + (unsigned long) start_index * (unsigned long) (opages + ipages) * (unsigned long) PAGE_SIZE_4K;
  out_cq_base_pa_len = (unsigned long)((unsigned long) opages * (unsigned long) PAGE_SIZE_4K);
  out_cq_base = (unsigned long) mmap(NULL, out_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, out_cq_base_pa);

  if(out_cq_base == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(console_fd) ;

    return (0);
  }

  console_channel->out_cq = (struct circular_queue *)(out_cq_base);
  console_channel->out_cq_len = out_cq_base_pa_len;

  // cq_init() will be done in unikernel
  // cf: cq_init(console_channel->out_cq, (opages - 1) / CQ_ELE_PAGE_NUM);

  in_cq_base_pa = (unsigned long) out_cq_base_pa + (unsigned long)((unsigned long) opages * (unsigned long) PAGE_SIZE_4K);
  in_cq_base_pa_len = (unsigned long) ((unsigned long) ipages * PAGE_SIZE_4K);
  in_cq_base = (unsigned long) mmap(NULL, in_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, in_cq_base_pa);

  if(in_cq_base == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    munmap(console_channel->out_cq, console_channel->out_cq_len);
    close(console_fd) ;

    return (0);
  }

  console_channel->in_cq = (struct circular_queue *)(in_cq_base);
  console_channel->in_cq_len = in_cq_base_pa_len;

  // cq_init() will be done in unikernel
  // cf: cq_init(console_channel->in_cq, (ipages - 1) / CQ_ELE_PAGE_NUM);

#ifdef DEBUG
    printf("out_cq_base_pa: %lx out_cq_base: %lx \n", (unsigned long) out_cq_base_pa, (unsigned long) out_cq_base);
    printf("in_cq_base_pa : %lx in_cq_base:  %lx \n", (unsigned long) in_cq_base_pa, (unsigned long) in_cq_base);
#endif

  // et console channel info ///////////////////////////////////////////////////
  console_channels_info = ((unsigned long *) g_mmap_console_channels_info_va) + start_index;
  *(console_channels_info) = (unsigned long) (CONSOLE_MAGIC + start_index);

  close(console_fd);

  return (1);
}


/**
 * @brief munmap unikernels' memory
 * @return success (1), fail (0)
 */
int munmap_unikernel_memory(void) {

  if(munmap((void *) g_mmap_kernel_mem_base_va, (unsigned long) MEMORYS_PER_NODE << 30) < 0)
    return (0);

  return (1);
}

/**
 * @brief mmap unikernels' memory
 * @return success (1), fail (0)
 */
int mmap_unikernel_memory(int start_index)
{
  unsigned long kernel_mem_base_pa_len;
  unsigned long channel_mem_base_pa_len;

  int console_fd = 0;

  console_fd = open("/dev/lk", O_RDWR | O_SYNC) ;
  if ( console_fd < 0 ) {
    printf("/dev/lk open error\n") ;

    return (0);
  }

  g_kernel_mem_base_pa = (unsigned long) UNIKERNELS_MEM_BASE_PA + (((unsigned long) (MEMORYS_PER_NODE * start_index)) << 30);
  kernel_mem_base_pa_len = ((unsigned long) MEMORYS_PER_NODE) << 30;

  g_mmap_kernel_mem_base_va = (unsigned long) mmap(NULL, kernel_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, g_kernel_mem_base_pa);

  if ( g_mmap_kernel_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(console_fd) ;

    return (0);
  }

  g_channel_mem_base_pa = (unsigned long) CONFIG_CHANNEL_PA;
  channel_mem_base_pa_len = (unsigned long) CHANNEL_SIZE << 30;

  g_mmap_channel_mem_base_va = (unsigned long) mmap(NULL, channel_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, console_fd, g_channel_mem_base_pa);

  if ( g_mmap_channel_mem_base_va == (unsigned long) MAP_FAILED ) {
    printf("mmap failed.\n") ;
    close(console_fd) ;

    return (0);
  }

  close(console_fd) ;

  return (1);
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

  if(pa > g_kernel_mem_base_pa) {
    offset = (unsigned long) (pa - g_kernel_mem_base_pa);
    return (g_mmap_kernel_mem_base_va + offset);
  }
  else {
    offset = (unsigned long) (pa - g_channel_mem_base_pa);
    return (g_mmap_channel_mem_base_va + offset);
  }

}


/**
 * @brief get unikernels' start physical address
 * @return (start physical address)
 */
unsigned long get_pa_base() 
{
  return (g_kernel_mem_base_pa);
}


/**
 * @brief get driver's start virutal address that matches start physical address of unikernel
 * @return (start virtual address)
 */
unsigned long get_va_base() 
{
  return (g_mmap_kernel_mem_base_va);
}


