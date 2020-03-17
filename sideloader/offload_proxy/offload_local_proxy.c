#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "arch.h"
#include "offload_channel.h"
#include "offload_mmap.h"
#include "offload_memory_config.h"
#include "offload_fio.h"
#include "offload_network.h"
#include "systemcalllist.h"
#include "offload_thread_pool.h"

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)

#define L_CACHE_LINE_SIZE       64

#define OFFLOAD_MAX_CHANNEL     (300)
#define MAX_MUTEX 		OFFLOAD_MAX_CHANNEL
#define MAX_NODE 		64 

//mmapped unikernels' start memory address, unikernels's start physical address = 48G
extern unsigned long g_mmap_unikernels_mem_base_va;

typedef struct offload_watch_arg_info {
   channel_t *ch;	
   int n_ch;
   int n_cq_element;
   int index;
} offload_watch_arg_t; 

int g_offload_channel_runnable;
int g_n_channels_per_node;
int g_n_channels;
int g_n_nodes;

// the max number of threas in thread pool
 #define MAX_POOL_THREAD	2048 

// mutex to handle send_offload_message() 
pthread_mutex_t job_mutex[MAX_MUTEX] __attribute__((aligned(L_CACHE_LINE_SIZE)));  

// data struct of thread argument for thread pool
typedef struct job_mutex {
  char padding[64] __attribute__((aligned(L_CACHE_LINE_SIZE)));
  pthread_mutex_t mutex __attribute__((aligned(L_CACHE_LINE_SIZE)));
} job_mutex_t;

// pointer to thread pool 
thread_pool_t *g_pool[MAX_NODE];

/**
 * @brief memcpy by sizeof(unsigned long) 
 * @param destination, source, size 
 * @return (copied size)
 */
int memcpy64(void *destination, const void *source, int size)
{
  int i = 0;
  int remain_byte = 0;

  for (i=0; i<(size / 8); i++)
    ((unsigned long *) destination)[i] = ((unsigned long *) source)[i];

  remain_byte = i * 8;
  for (i=0; i<(size % 8); i++) {
    ((char *) destination)[remain_byte] = ((char *) source)[remain_byte];
    remain_byte++;
  }
  return size;
}

/**
 * @brief offload local proxy thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *offload_local_proxy(void *arg)
{
io_packet_t *in_pkt = NULL;
job_args_t *job_args = NULL;

  job_args = (job_args_t *) arg;
  in_pkt = (io_packet_t *) &job_args->pkt;
      
  switch(in_pkt->io_function_type) {
    case SYSCALL_sys_read:
       sys_off_read(job_args);
       break;
    case SYSCALL_sys_write:
       sys_off_write(job_args);
       break;
    case SYSCALL_sys_lseek:
       sys_off_lseek(job_args);
       break;
    case SYSCALL_sys_open:
       sys_off_open(job_args);
       break;
    case SYSCALL_sys_creat:
       sys_off_creat(job_args);
       break;
    case SYSCALL_sys_close:
       sys_off_close(job_args);
       break;
    case SYSCALL_sys_link:
       sys_off_link(job_args);
       break;
    case SYSCALL_sys_unlink:
       sys_off_unlink(job_args);
       break;
    case SYSCALL_sys_gethostname:
       sys_off_gethostname(job_args);
       break;
    case SYSCALL_sys_gethostbyname:
       sys_off_gethostbyname(job_args);
       break;
    case SYSCALL_sys_getsockname:
       sys_off_getsockname(job_args);
       break;
    case SYSCALL_sys_socket:
       sys_off_socket(job_args);
       break;
    case SYSCALL_sys_bind:
       sys_off_bind(job_args);
       break;
    case SYSCALL_sys_listen:
       sys_off_listen(job_args);
       break;
    case SYSCALL_sys_connect:
       sys_off_connect(job_args);
       break;
    case SYSCALL_sys_accept:
       sys_off_accept(job_args);
       break;
    case SYSCALL_sys_stat:
       sys_off_stat(job_args);
       break;
    case SYSCALL_sys3_getcwd:
       sys3_off_getcwd(job_args);
       break;
    case SYSCALL_sys3_system:
       sys3_off_system(job_args);
       break;
    case SYSCALL_sys_chdir:
       sys_off_chdir(job_args);
       break;
    case SYSCALL_sys3_opendir:
       sys3_off_opendir(job_args);
       break;
    case SYSCALL_sys3_closedir:
       sys3_off_closedir(job_args);
       break;
    case SYSCALL_sys3_readdir:
       sys3_off_readdir(job_args);
       break;
    case SYSCALL_sys3_rewinddir:
       sys3_off_rewinddir(job_args);
       break;
    default :
       printf("function type: unknown[%d]\n", (int) in_pkt->io_function_type);
       break;
  }

  return (NULL);
}


/**
 * @brief offload local watch thread
 * @param arg contains channels and channel number for this thread 
 * @return none
 */
void *offload_watch(void *arg)
{
  struct offload_watch_arg_info *thread_channels = (struct offload_watch_arg_info *)arg;

  cq_element *ce = NULL;
  io_packet_t *in_pkt = NULL;

  int n_channels = 0;
  int n_cq_element= 0;

  struct channel_struct *offload_channels = NULL;
  struct channel_struct *curr_channel = NULL;
  struct circular_queue *in_cq = NULL;

  int i = 0;

  //thread job parameer
  job_args_t *job_args = NULL;
  int max_job_size = 0;
  int job_args_index = 0;
  int thread_pool_size = 0;
  thread_pool_t *pool = NULL;
  int watch_count = 0;

  offload_channels = thread_channels->ch;
  n_channels = thread_channels->n_ch;
  n_cq_element = thread_channels->n_cq_element;

#ifdef DEBUG
  printf("# channels: %d, # cq_element: %d\n", n_channels, n_cq_element);
#endif

  //alloc memory for job parameter
  max_job_size = n_channels * n_cq_element;
  job_args = (job_args_t *) malloc(max_job_size * sizeof(job_args_t));
  if (job_args == NULL) {
    printf("failed to allocate memory\n");
    exit(1);
  }

  //create thread pool
  thread_pool_size = MAX_POOL_THREAD;
  pool = create_pool(thread_pool_size);
  g_pool[thread_channels->index] = pool;
  sleep(1);

  // initialize mutex for  send_offload_message()
  for(i = 0; i < n_channels; i++) {
    pthread_mutex_init(&(offload_channels + i)->mutex, NULL);
  }

#define RETRY

#ifdef RETRY
  int retry_count = 0;
  int max_retry_count = 0;
#endif

  job_args_index = 0;
  watch_count = 0;

  while (g_offload_channel_runnable) {

    if(watch_count++ > 1000) {
      //sleep watch thread
      usleep(1);
      watch_count = 0;
    }

    for(i = 0; i < n_channels && g_offload_channel_runnable; i++) {
        curr_channel = (struct channel_struct *) (offload_channels + i);
        in_cq = (struct circular_queue *) curr_channel->in_cq;

#ifdef RETRY
        retry_count = 0;
        max_retry_count = cq_avail_data(in_cq);
#endif
retry_watch:

        if(cq_avail_data(in_cq)) {
          watch_count = 0;
          ce = (cq_element *) (in_cq->data + in_cq->tail);
          in_pkt= (io_packet_t *)(ce);

          // copy pkt to thread args
          memcpy64((void *)&job_args[job_args_index].pkt, (void *) in_pkt, sizeof(io_packet_t));
          job_args[job_args_index].ch = curr_channel;

          in_cq->tail = (in_cq->tail + 1) % in_cq->size;

          //add job to thread pool
          while(WAIT_ISSUED == add_job_to_pool(pool, (void *) offload_local_proxy, (void *) &job_args[job_args_index])) {
	    usleep(1);
          } 

          job_args_index = (job_args_index + 1) % max_job_size;

#ifdef RETRY 
          if(retry_count++ < max_retry_count) {
            goto retry_watch;
          }
#endif
        }
    }
  }

  //destroy thread pool
  destroy_pool(pool);

  //free memory for job args
  free(job_args);

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
          printf("queue state(%03d): [p->u(%3d): h:%3d t:%3d] [u->p(%3d): h:%3d t:%3d]\n", i, cq_avail_data(ocq), ocq->head, ocq->tail, cq_avail_data(icq), icq->head, icq->tail);
        }
        break;
 
      // show thread pool status
      case 't':
        for (i = 0; i < g_n_nodes; i++) {
          printf("thread pool: #threads: %d, #pending jobs: %d \n", (int) get_thread_count(g_pool[i]), (int) get_job_count(g_pool[i]));
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
 *              " <no channels per unikernel> -n <no unikernels>
 * @return  success 0, fail > 0
 */
int main(int argc, char *argv[])
{
  channel_t *offload_channels;
  offload_watch_arg_t offload_watch_arg[MAX_NODE];
  unsigned long ocq_elements, icq_elements;
  unsigned long opages, ipages;
  int err = 0;
  int i = 0;
  unsigned long status;
  pthread_t *threads_offload_watch;

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

  threads_offload_watch = (pthread_t *) malloc((g_n_nodes) * sizeof(pthread_t)); // 1 for watch
  if (threads_offload_watch == NULL) {
    printf("failed to allocate memory\n");
    goto __free_threads_offload_watch;
  }

  for (i = 0; i < g_n_channels; i++)
    init_channel(offload_channels + i);

  printf("#nodes = %d #channels = %d #out elements = %d #in elements = %d \n", (int) g_n_nodes, (int) g_n_channels, (int) ocq_elements, (int) icq_elements);
  if (!mmap_channels(offload_channels, g_n_nodes, g_n_channels, opages, ipages))
      err++;

  if (err)
    goto __end;

  g_offload_channel_runnable = 1;

#define MULTI_WATCH

#ifdef MULTI_WATCH
  for (i = 0; i < g_n_nodes; i++) {
    offload_watch_arg[i].ch = offload_channels + i * g_n_channels_per_node;
    offload_watch_arg[i].n_ch = g_n_channels_per_node;
    offload_watch_arg[i].n_cq_element = icq_elements;
    offload_watch_arg[i].index = i;
    pthread_create(threads_offload_watch + i, NULL, offload_watch, &offload_watch_arg[i]);
  }
#else
    offload_watch_arg[0].ch = offload_channels;
    offload_watch_arg[0].n_ch = g_n_channels;
    offload_watch_arg[0].n_cq_element = icq_elements;
    offload_watch_arg[0].index = 0;
    pthread_create(threads_offload_watch + 0, NULL, offload_watch, &offload_watch_arg[0]);

#endif

  // begin of logic
  cmd(offload_channels);

#ifdef MULTI_WATCH
  for (i = 0; i < g_n_nodes; i++) { // 1 for watch
#else
  for (i = 0; i < 1; i++) { // 1 for watch
#endif
    pthread_join(*(threads_offload_watch + i), (void **)&status);
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

__free_threads_offload_watch:
  if (threads_offload_watch)
    free(threads_offload_watch);

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
