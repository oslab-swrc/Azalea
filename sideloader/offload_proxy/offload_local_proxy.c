#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "offload_channel.h"
#include "offload_mmap.h"
#include "offload_memory_config.h"
#include "offload_fio.h"
#include "offload_network.h"
#include "systemcalllist.h"
#include "arch.h"

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)

#define L_CACHE_LINE_SIZE       64

#define OFFLOAD_MAX_CHANNEL     (300)
#define MAX_MUTEX 		OFFLOAD_MAX_CHANNEL
#define MAX_NODE 		64 

//mmapped unikernels' start memory address, unikernels's start physical address = 48G
extern unsigned long g_mmap_unikernels_mem_base_va;

struct thread_channel_information {
   channel_t *ch;	
   int n_ch;
   int mutex_index;
}; 

int g_offload_channel_runnable;
int g_n_channels_per_node;
int g_n_channels;
int g_n_nodes;

pthread_mutex_t count_mutex[MAX_MUTEX] __attribute__((aligned(L_CACHE_LINE_SIZE)));  
pthread_cond_t count_threshold_cv[MAX_MUTEX] __attribute__((aligned(L_CACHE_LINE_SIZE))); 
int is_data[MAX_MUTEX] __attribute__((aligned(L_CACHE_LINE_SIZE)));
int watch_data[MAX_MUTEX] __attribute__((aligned(L_CACHE_LINE_SIZE)));

volatile int atomic_kids;

/**
 * @brief offload local proxy thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *offload_local_proxy(void *arg)
{
  struct thread_channel_information *thread_channels = (struct thread_channel_information *)arg;

  struct channel_struct *offload_channel = NULL;
  struct channel_struct *curr_channel = NULL;
  struct circular_queue *in_cq = NULL;
  io_packet_t *in_pkt = NULL;

  int index = 0;

  offload_channel = thread_channels->ch;
  index = thread_channels->mutex_index;

  curr_channel = offload_channel;
  in_cq = (struct circular_queue *) curr_channel->in_cq;


  __sync_fetch_and_add(&atomic_kids, 1);

  while (g_offload_channel_runnable) {

    pthread_mutex_lock(&count_mutex[index]);

    while(is_data[index] == 0) {
      pthread_cond_wait(&count_threshold_cv[index],  &count_mutex[index]); 
    }

    while(cq_avail_data(in_cq)) {
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

    is_data[index] = 0;
    watch_data[index] = 1;
    pthread_mutex_unlock(&count_mutex[index]);
  }

  pthread_exit((void *)0);
  return (NULL);
}


/**
 * @brief offload local watch thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *offload_local_watch(void *arg)
{
  struct thread_channel_information *thread_channels = (struct thread_channel_information *)arg;

  int n_channels_of_thread = 0;
  int mutex_index = 0;
  struct channel_struct *offload_channels = NULL;
  struct channel_struct *curr_channel = NULL;
  struct circular_queue *in_cq = NULL;

  int i = 0;

  offload_channels = thread_channels->ch;
  n_channels_of_thread = thread_channels->n_ch;
  mutex_index = thread_channels->mutex_index;

  while (g_offload_channel_runnable) {
    for(i = 0; i < n_channels_of_thread && g_offload_channel_runnable; i++) {
      if(watch_data[mutex_index + i] == 1) {
        curr_channel = (struct channel_struct *) (offload_channels + i);
        asm volatile ("" : : : "memory");
        in_cq = (struct circular_queue *) curr_channel->in_cq;
        if(cq_avail_data(in_cq)) {
          pthread_mutex_lock(&count_mutex[mutex_index + i]);
          is_data[mutex_index + i] = 1;
          watch_data[mutex_index + i] = 0;
          pthread_cond_signal(&count_threshold_cv[mutex_index + i]);  
          pthread_mutex_unlock(&count_mutex[mutex_index + i]); 
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
        if((i%g_n_channels_per_node == 0)&& (i != 0))
        printf("\n");
        printf("queue state(%03d): [p->u: h:%2d t:%2d] [u->p: h:%2d t:%2d]\n", i, ocq->head, ocq->tail, icq->head, icq->tail);
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
  struct thread_channel_information thread_channels_watch[MAX_NODE];
  unsigned long ocq_elements, icq_elements;
  unsigned long opages, ipages;
  int err = 0;
  int i = 0;
  unsigned long status;
  pthread_t *offload_threads;
  pthread_t *offload_threads_watch;

  unsigned long unikernels_mem_size = 0;

  if (argc != 9) {
    printf("usage: ./offload_local_proxy -o <no elements> -i <no elements> -c"
	   " <no channels per node> -n <no nodes>\n");
    exit(1);
  }

  ocq_elements = atol(argv[2]);
  icq_elements = atol(argv[4]);
  opages = ocq_elements * CQ_ELE_PAGE_NUM + 1; // payload = CQ_ELE_PAGE_NUM pages, metadata = 1 page
  ipages = icq_elements * CQ_ELE_PAGE_NUM + 1;

  g_n_channels_per_node = atoi(argv[6]);

  g_n_nodes = atoi(argv[8]);

  g_n_channels = g_n_channels_per_node * g_n_nodes;

  unikernels_mem_size = mmap_unikernels_memory(g_n_nodes);

  if(unikernels_mem_size == 0) 
	  goto __exit; 

  if ((opages * PAGE_SIZE) <= 0 || (opages * PAGE_SIZE) > LONG_MAX ||
	  (ipages * PAGE_SIZE) <= 0 || (ipages * PAGE_SIZE) > LONG_MAX) {
    printf("not valid msg size");
    exit(1);
  }

  offload_channels = (channel_t *) malloc(g_n_channels * sizeof(channel_t));
  if (offload_channels == NULL) {
    printf("failed to allocate memory\n");
    exit(1);
  }

  offload_threads = (pthread_t *) malloc((g_n_channels) * sizeof(pthread_t)); // 1 for watch
  if (offload_threads == NULL) {
    printf("failed to allocate memory\n");
    goto __free_offload_threads;
  }

  offload_threads_watch = (pthread_t *) malloc((g_n_nodes) * sizeof(pthread_t)); // 1 for watch
  if (offload_threads_watch == NULL) {
    printf("failed to allocate memory\n");
    goto __free_offload_threads_watch;
  }

  for (i = 0; i < g_n_channels; i++)
    init_channel(offload_channels + i);

  printf("#nodes = %d #channels = %d #out elements = %d #in elements = %d \n", (int) g_n_nodes, (int) g_n_channels, (int) ocq_elements, (int) icq_elements);
  if (!mmap_channels(offload_channels, g_n_nodes, g_n_channels, opages, ipages))
      err++;

  if (err)
    goto __end;

  g_offload_channel_runnable = 1;

  for(i = 0; i < MAX_MUTEX; i++) {
    pthread_mutex_init(&count_mutex[i], NULL);  
    pthread_cond_init(&count_threshold_cv[i], NULL);
    is_data[i] = 0;
    watch_data[i] = 1;
  }

  atomic_kids = 0;

  channel_t *curr_ch = offload_channels;
  for (i = 0; i < g_n_channels; i++) {
      thread_channels[i].ch = curr_ch;
      thread_channels[i].n_ch = 1;
      thread_channels[i].mutex_index = i;
      pthread_create(offload_threads + i, NULL, offload_local_proxy, &thread_channels[i]);
      curr_ch++;
  }

  // wait for offload_local_proxy
  while(1) {
    if(atomic_kids == g_n_channels) 
     break;
  }  

  usleep(1000);

#if 0
  for (i = 0; i < g_n_nodes; i++) {
    thread_channels_watch[i].ch = offload_channels + i * g_n_channels_per_node;
    thread_channels_watch[i].n_ch = g_n_channels_per_node;
    thread_channels_watch[i].mutex_index = i * g_n_channels_per_node;
    pthread_create(offload_threads_watch + i, NULL, offload_local_watch, &thread_channels_watch[i]);
  }
#else
    thread_channels_watch[0].ch = offload_channels;
    thread_channels_watch[0].n_ch = g_n_channels;
    thread_channels_watch[0].mutex_index = 0;
    pthread_create(offload_threads_watch + 0, NULL, offload_local_watch, &thread_channels_watch[0]);

#endif

  // begin of logic
  cmd(offload_channels);

  for (i = 0; i < g_n_channels; i++) { // 1 for watch
    pthread_mutex_lock(&count_mutex[i]);
    pthread_cond_signal(&count_threshold_cv[i]);  
    is_data[i] = 1;
    pthread_mutex_unlock(&count_mutex[i]); 
  }

  for (i = 0; i < g_n_channels; i++) { // 1 for watch
    pthread_join(*(offload_threads + i), (void **)&status);
  }

#if 0
  for (i = 0; i < g_n_nodes; i++) { // 1 for watch
#else
  for (i = 0; i < 1; i++) { // 1 for watch
#endif
    pthread_join(*(offload_threads_watch + i), (void **)&status);
  }

  for (i = 0; i < g_n_channels; i++) { // 1 for watch
    pthread_mutex_destroy(&count_mutex[i]);
    pthread_cond_destroy(&count_threshold_cv[i]);
  }
  // end of logic

  if (munmap_channels(offload_channels, g_n_channels) < 0)
      err++;

  if (err) {
    printf("%s failed to unregister channel \n", __func__);
    goto __end;
  }

  errno = 0;

__end:

__free_offload_threads_watch:
  if (offload_threads_watch)
    free(offload_threads_watch);

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
