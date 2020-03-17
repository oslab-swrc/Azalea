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

#define	N_CHANNEL	MAX_UNIKERNEL	

//mmapped unikernels' start memory address, unikernels's start physical address = 48G
extern unsigned long g_mmap_kernel_mem_base_va;
extern unsigned long g_mmap_console_channels_info_va;

int g_console_proxy_runnable;
int g_n_channels;
int g_n_threads;
int g_n_nodes;
int start_index;

/**
 * @brief console local proxy thread
 * @param arg contains channels and channel number for this thread 
 * @return (NULL)
 */
void *console_proxy(void *args)
{
  struct channel_struct *console_channel = NULL;
  struct circular_queue *in_cq = NULL;
  io_packet_t *in_pkt = NULL;

  //console_channel = thread_channels->ch;
  console_channel = (struct channel_struct *) args;
  in_cq = (struct circular_queue *) console_channel->in_cq;

  while (g_console_proxy_runnable) {

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
	     g_console_proxy_runnable = 0;
             break;
          default :
             printf("function type: unknown[%d]\n", (int) in_pkt->io_function_type);
             break;
        }
      }
      else {
        usleep(1);
      }
  }

  pthread_exit((void *) 0);
  return (NULL);
}


/**
 * @brief wait command and do command
 * @param  channel
  @return none
 */
void cmd(channel_t *cs)
{
  struct circular_queue *ocq;
  struct circular_queue *icq;

  int loop = 1;
  unsigned long *console_channel_info = NULL;

  while (loop) {
    switch (getchar()) {
    // show queue status
      case 'q':
          ocq = cs->out_cq;
          icq = cs->in_cq;
          printf("queue state(%03d): ptu: h:%d t:%d utp: h:%d t:%d\n", start_index, ocq->head, ocq->tail, icq->head, icq->tail);
        break;
 
      // stop console_local_proxy which manages the channels on scif region
      case 'x':
        g_console_proxy_runnable = 0;
        console_channel_info = ((unsigned long *) g_mmap_console_channels_info_va) + start_index;
        *(console_channel_info) = (unsigned long) 0;
        loop = 0;
        break;
 
      case '\n':
        printf("> ");
        fflush(stdout);
        break;
 
      default:
        break;
    }
    usleep(10);
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
  unsigned long ocq_elements = 0;
  unsigned long icq_elements = 0;
  unsigned long opages = 0;
  unsigned long ipages = 0;

  pthread_t console_thread;
  unsigned long status = 0;

  int err = 0;

  if (argc != 3) {
    printf("usage: ./console_console -i [index]\n");
    exit(1);
  }

  start_index = atol(argv[2]);

  ocq_elements = CONSOLE_CHANNEL_CQ_ELEMENT_NUM;
  icq_elements = CONSOLE_CHANNEL_CQ_ELEMENT_NUM;
  opages = ocq_elements * CQ_ELE_PAGE_NUM + 1; // payload = CQ_ELE_PAGE_NUM pages, metadata = 1 page
  ipages = icq_elements * CQ_ELE_PAGE_NUM + 1;
  g_n_channels = N_CHANNEL;

  if(!mmap_unikernel_memory(start_index))
    goto __exit;

  if(err)
    goto __exit;

  if ((opages * PAGE_SIZE) <= 0 || (opages * PAGE_SIZE) > LONG_MAX ||
	  (ipages * PAGE_SIZE) <= 0 || (ipages * PAGE_SIZE) > LONG_MAX) {
    printf("not valid msg size\n");
    exit(1);
  }

  // init console channel
  init_channel(&console_channel);

  // mmap console channel
  if (!mmap_console_channel(&console_channel, start_index, opages, ipages))
    err++;

  if (err)
    goto __end;

  //set console proxy status
  g_console_proxy_runnable = 1;

  // console proxy thread
  pthread_create(&console_thread, NULL, console_proxy, &console_channel);

  // begin of logic
  cmd(&console_channel);
  // end of logic

  pthread_join(console_thread, (void **)&status);

  //unmmap console channel
  if(!munmap_console_channel(&console_channel)) {
    err++;
    printf("%s failed to unregister channel \n", __func__);
    goto __end;
  } 

  //unmmap unikernel memory 
  if(!munmap_unikernel_memory() < 0){
    err++;
    printf("%s failed to unregister channel \n", __func__);
    goto __end;
   }

  errno = 0;
__end:

  if (errno == 0)
    printf("\n");
  else
    printf("======== Program Failed ========\n");


__exit:

  return err;
}
