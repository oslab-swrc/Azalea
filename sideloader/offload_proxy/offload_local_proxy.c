#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "offload_channel.h"
#include "offload_mmap.h"
#include "offload_memory_config.h"
#include "offload_fio.h"
#include "offload_network.h"
#include "systemcalllist.h"
#include "arch.h"

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)

#define OFFLOAD_MAX_CHANNEL     (300)

//mmapped unikernels' start memory address, unikernels's start physical address = 48G
extern unsigned long g_mmap_unikernels_mem_base_va;

struct thread_channel_information {
   channel_t *ch;	
   int n_ch;
}; 

int g_offload_channel_runnable;
int g_n_channels;
int g_n_threads;
int g_n_nodes;

/**
 * @brief offload local proxy thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *offload_local_proxy(void *arg)
{
  struct thread_channel_information *thread_channels = (struct thread_channel_information *)arg;

  int n_channels_of_thread = 0;
  struct channel_struct *offload_channels = NULL;
  struct channel_struct *curr_channel = NULL;
  struct circular_queue *in_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int i = 0;

  offload_channels = thread_channels->ch;
  n_channels_of_thread = thread_channels->n_ch;

  while (g_offload_channel_runnable) {

    for(i = 0; i < n_channels_of_thread && g_offload_channel_runnable; i++) {

      curr_channel = (struct channel_struct *) (offload_channels + i);
      in_cq = (struct circular_queue *) curr_channel->in_cq;

      if(cq_avail_data(in_cq)) {

        in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);
        
        switch(in_pkt->io_function_type) {
          case SYSCALL_sys_read:
             sys_off_read(curr_channel);
             break;
          case SYSCALL_sys_write:
             sys_off_write(curr_channel);
             break;
          case SYSCALL_sys_lseek:
             sys_off_lseek(curr_channel);
             break;
          case SYSCALL_sys_open:
             sys_off_open(curr_channel);
             break;
          case SYSCALL_sys_creat:
             sys_off_creat(curr_channel);
             break;
          case SYSCALL_sys_close:
             sys_off_close(curr_channel);
             break;
          case SYSCALL_sys_link:
             sys_off_link(curr_channel);
             break;
          case SYSCALL_sys_unlink:
             sys_off_unlink(curr_channel);
             break;
          case SYSCALL_sys_gethostname:
             sys_off_gethostname(curr_channel);
             break;
          case SYSCALL_sys_gethostbyname:
             sys_off_gethostbyname(curr_channel);
             break;
          case SYSCALL_sys_getsockname:
             sys_off_getsockname(curr_channel);
             break;
          case SYSCALL_sys_socket:
             sys_off_socket(curr_channel);
             break;
          case SYSCALL_sys_bind:
             sys_off_bind(curr_channel);
             break;
          case SYSCALL_sys_listen:
             sys_off_listen(curr_channel);
             break;
          case SYSCALL_sys_connect:
             sys_off_connect(curr_channel);
             break;
          case SYSCALL_sys_accept:
             sys_off_accept(curr_channel);
             break;
          case SYSCALL_sys_stat:
             sys_off_stat(curr_channel);
             break;
          case SYSCALL_sys3_getcwd:
             sys3_off_getcwd(curr_channel);
             break;
          case SYSCALL_sys3_system:
             sys3_off_system(curr_channel);
             break;
          case SYSCALL_sys_chdir:
             sys_off_chdir(curr_channel);
             break;
          case SYSCALL_sys3_opendir:
             sys3_off_opendir(curr_channel);
             break;
          case SYSCALL_sys3_closedir:
             sys3_off_closedir(curr_channel);
             break;
          case SYSCALL_sys3_readdir:
             sys3_off_readdir(curr_channel);
             break;
          case SYSCALL_sys3_rewinddir:
             sys3_off_rewinddir(curr_channel);
             break;
          default :
             printf("function type: unknown[%d]\n", (int) in_pkt->io_function_type);
             break;
        }
      }
    }
  }

  pthread_exit((void *)0);
  return (NULL);
}


/**
 * @brief wait command and do command
 * @param  channel
 * @return none
 */
void cmd(channel_t *cs)
{
  struct circular_queue *ocq;
  struct circular_queue *icq;

  int loop = 1;
  int i = 0;

  printf("> ");
  fflush(stdout);

  while (loop) {
    switch (getchar()) {
    // show queue status
    case 'q':
      for (i = 0; i < g_n_channels; i++) {
		ocq = (cs + i)->out_cq;
		icq = (cs + i)->in_cq;
        printf("queue state(%03d): ptu: h:%d t:%d utp: h:%d t:%d\n", i, ocq->head, ocq->tail, icq->head, icq->tail);
      }
      break;

    // stop offload_local_proxy which manages the channels on scif region
    case 'x':
      g_offload_channel_runnable = 0;
	  loop = 0;
      break;

    case '\n':
      printf("> ");
      fflush(stdout);
      break;

    default:
      break;
    }
  }
}


/**
 * @brief offload local proxy main
 * @param argc
 * @param argv  "-o <no elements> -i <no elements> -c"
 *              " <no channels> -t <no threads>
 * @return  success 0, fail > 0
 */
int main(int argc, char *argv[])
{
  channel_t *offload_channels;
  struct thread_channel_information thread_channels[OFFLOAD_MAX_CHANNEL];
  unsigned long ocq_elements, icq_elements;
  unsigned long opages, ipages;
  int err = 0;
  int i = 0;
  int j = 0;
  int k = 0;
  unsigned long status;
  pthread_t *offload_threads;
  int n_channels_per_node = 0;
  int n_threads_per_node = 0;
  unsigned long unikernels_mem_size = 0;

  if (argc != 11) {
    printf("usage: ./offload_local_proxy -o <no elements> -i <no elements> -c"
	   " <no channels per node> -t <no threads per node> -n <no nodes>\n");
    exit(1);
  }

  ocq_elements = atol(argv[2]);
  icq_elements = atol(argv[4]);
  opages = ocq_elements * CQ_ELE_PAGE_NUM + 1; // payload = CQ_ELE_PAGE_NUM pages, metadata = 1 page
  ipages = icq_elements * CQ_ELE_PAGE_NUM + 1;

  n_channels_per_node = atoi(argv[6]);
  n_threads_per_node = atoi(argv[8]);
  g_n_nodes = atoi(argv[10]);
  if(g_n_nodes <= 0) {
    printf("Error: Requested node number is %d.", g_n_nodes);
    exit(1);
  }

  g_n_channels = n_channels_per_node * g_n_nodes;
  g_n_threads = n_threads_per_node * g_n_nodes;

  unikernels_mem_size = mmap_unikernels_memory(g_n_nodes);

  if(unikernels_mem_size == 0) 
	  goto __exit; 

  if ((opages * PAGE_SIZE) <= 0 || (opages * PAGE_SIZE) > LONG_MAX ||
	  (ipages * PAGE_SIZE) <= 0 || (ipages * PAGE_SIZE) > LONG_MAX) {
    printf("not valid msg size");
    exit(1);
  }

  offload_channels = malloc(g_n_channels * sizeof(channel_t));
  if (offload_channels == NULL) {
    printf("failed to allocate memory\n");
    exit(1);
  }

  offload_threads = malloc(g_n_threads * sizeof(pthread_t));
  if (offload_threads == NULL) {
    printf("failed to allocate memory\n");
    goto __free_offload_threads;
  }

  for (i = 0; i < g_n_channels; i++)
    init_channel(offload_channels + i);

  printf("#nodes = %d #channels = %d #out elements = %d #in elements = %d \n", (int) g_n_nodes, (int) g_n_channels, (int) ocq_elements, (int) icq_elements);
  if (!mmap_channels(offload_channels, g_n_nodes, g_n_channels, opages, ipages))
      err++;

  if (err)
    goto __end;

  g_offload_channel_runnable = 1;

  int quotient_channel = n_channels_per_node / n_threads_per_node;
  int rest_channel = 0;

  channel_t *curr_ch = offload_channels;
  channel_t *next_ch = NULL;
  k = 0;
  for (i = 0; i < g_n_nodes; i++) {

    rest_channel = n_channels_per_node % n_threads_per_node;
    for (j = 0; j < n_threads_per_node; j++) {

      if(rest_channel == 0) {
        thread_channels[k].ch = curr_ch;
        thread_channels[k].n_ch = quotient_channel;
        next_ch = curr_ch + quotient_channel;
      }
      else {
        thread_channels[k].ch = curr_ch;
        thread_channels[k].n_ch = quotient_channel + 1;
        next_ch = curr_ch + (quotient_channel + 1);
        rest_channel--;
      }
      pthread_create(offload_threads + k, NULL, offload_local_proxy, &thread_channels[k]);

      k++;
      curr_ch = next_ch;
    }
  }

  // begin of logic
  cmd(offload_channels);

  for (i = 0; i < g_n_threads; i++) {
    pthread_join(*(offload_threads + i), (void **)&status);
  }
  // end of logic

//__unregister:

  if (munmap_channels(offload_channels, g_n_channels) < 0)
      err++;

  if (err) {
    printf("%s failed to unregister channel \n", __func__);
    goto __end;
  }

  errno = 0;

__end:

__free_offload_threads:
  if (offload_threads)
    free(offload_threads);

  if (offload_channels)
    free(offload_channels);

  if (errno == 0)
    printf("======== Offload proxy finished ========\n");
  else
    printf("======== Program Failed ========\n");

  if(g_mmap_unikernels_mem_base_va != 0)
    munmap((void *) g_mmap_unikernels_mem_base_va, unikernels_mem_size); 

__exit:

  return err;
}
