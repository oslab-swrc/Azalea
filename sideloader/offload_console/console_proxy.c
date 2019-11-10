#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <curses.h>
#include <unistd.h>

#include "console_channel.h"
#include "console_mmap.h"
#include "console_memory_config.h"
#include "console_function.h"
#include "systemcalllist.h"
#include "arch.h"

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)

#define	N_CHANNEL_PER_NODE	1

//mmapped unikernels' start memory address, unikernels's start physical address = 48G
extern unsigned long g_mmap_unikernel_mem_base_va;

struct thread_channel_information {
   channel_t *ch;	
   int n_ch;
}; 

int g_console_channel_runnable;
int g_n_channels;
int g_n_threads;
int g_n_nodes;
int g_node_id;

/**
 * @brief console local proxy thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *console_proxy(void *arg)
{
  struct thread_channel_information *thread_channels = (struct thread_channel_information *)arg;

  struct channel_struct *console_channel = NULL;
  struct circular_queue *in_cq = NULL;
  io_packet_t *in_pkt = NULL;

  console_channel = thread_channels->ch;
  in_cq = (struct circular_queue *) console_channel->in_cq;
  while (g_console_channel_runnable) {

      if(cq_avail_data(in_cq)) {

        in_pkt = (io_packet_t *) (in_cq->data + in_cq->tail);
        
        switch(in_pkt->io_function_type) {
          case CONSOLE_PRINT: 
	     console_print(console_channel);
             break;
          case CONSOLE_WRITE: 
	     console_off_write(console_channel);
             break;
          case CONSOLE_GETCH: 
	     console_getch(console_channel);
             break;
          case CONSOLE_EXIT: 
	     g_console_channel_runnable = 0;
	     //exit(0);
             break;
          default :
             printf("function type: unknown[%d]\n", (int) in_pkt->io_function_type);
             break;
        }
      }
      else {
        //__asm volatile ("pause" ::: "memory");
        usleep(1);
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

  //printf("> ");
  //fflush(stdout);

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

    // stop console_local_proxy which manages the channels on scif region
    case 'x':
      g_console_channel_runnable = 0;
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
 * @brief console proxy main
 * @param argc
 * @param argv  "-i [index]"
 * @return  success 0, fail > 0
 */
int main(int argc, char *argv[])
{
  channel_t console_channel;
  struct thread_channel_information thread_arg;
  unsigned long ocq_elements, icq_elements;
  unsigned long opages, ipages;
  int err = 0;
  int i = 0;
  unsigned long status;
  pthread_t console_thread;

  if (argc != 3) {
    printf("usage: ./console_console -i [index]\n");
    exit(1);
  }

  g_node_id = atol(argv[2]);

  ocq_elements = CONSOLE_CHANNEL_CQ_ELEMENT_NUM;
  icq_elements = CONSOLE_CHANNEL_CQ_ELEMENT_NUM;
  opages = ocq_elements * CQ_ELE_PAGE_NUM + 1; // payload = CQ_ELE_PAGE_NUM pages, metadata = 1 page
  ipages = icq_elements * CQ_ELE_PAGE_NUM + 1;
  g_n_channels = N_CHANNEL_PER_NODE;

  err = mmap_unikernel_memory(g_node_id);

  if(err)
          goto __exit;

  if ((opages * PAGE_SIZE) <= 0 || (opages * PAGE_SIZE) > LONG_MAX ||
	  (ipages * PAGE_SIZE) <= 0 || (ipages * PAGE_SIZE) > LONG_MAX) {
    printf("not valid msg size");
    exit(1);
  }


  init_channel(&console_channel);
  if (!mmap_console_channel(&console_channel, g_node_id, g_n_channels, opages, ipages))
      err++;

  if (err)
    goto __end;

  g_console_channel_runnable = 1;

  // console proxy threadt
  thread_arg.ch = (channel_t *)(&console_channel);
  thread_arg.n_ch = 1;
  pthread_create(&console_thread, NULL, console_proxy, &thread_arg);

  // begin of logic
  //cmd(&console_channel);

  for (i = 0; i < 1; i++) {
    pthread_join(console_thread, (void **)&status);
  }
  // end of logic

//__unregister:

  if (munmap_console_channel(&console_channel) < 0)
      err++;

  if (err) {
    printf("%s failed to unregister channel \n", __func__);
    goto __end;
  }

  errno = 0;

__end:

//__free_console_threads:

  if (errno == 0)
    printf("\n");
  else
    printf("======== Program Failed ========\n");

  if(g_mmap_unikernel_mem_base_va != 0)
    munmap((void *) g_mmap_unikernel_mem_base_va, MEMORYS_PER_NODE * 1024UL*1024UL*1024UL); 

__exit:

  return err;
}
