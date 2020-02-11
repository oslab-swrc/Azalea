#include "console.h"
#include "console_mmap.h"
#include "console_memory_config.h"
#include "memory.h"
#include "offload_channel.h"
#include "offload_page.h"
#include "page.h"
#include "utility.h"

#include <sys/lock.h>

//static channel_t g_console_channels[1];
static channel_t *g_console_channels;
static int g_console_id = 0;
BOOL g_console_proxy_flag = FALSE;
volatile QWORD *g_console_magic = NULL;

/**
 * @brief get console id 
 * @return console id
 */
int get_console_id(void) 
{
int current_memory_start = 0;
int global_memory_start = 0;
int memory_per_node = 0;
int console_id = 0;

  current_memory_start = *(QWORD*) (CONFIG_MEM_START + CONFIG_PAGE_OFFSET);
  global_memory_start =  (int) UNIKERNEL_START;
  memory_per_node =  (int) MEMORYS_PER_NODE;
  console_id =  (int) ((current_memory_start - global_memory_start) / memory_per_node);

  return (console_id);
}


/**
 * @brief init console 
 * @return success (TRUE), fail (FALSE)
 */
BOOL init_console_channel(void)
{
channel_t *console_channel;
int console_channel_offset = 0;

QWORD n_ipages = 0;
QWORD n_opages = 0;
QWORD icq_base_va = 0;
QWORD ocq_base_va = 0;
QWORD console_channel_info_va = 0;
QWORD console_channel_base_va = 0;

  g_console_id = get_console_id();

  g_console_channels = (channel_t *) ((QWORD) CONSOLE_CHANNEL_STRUCT_VA);
  console_channel_info_va = (QWORD) (CONSOLE_CHANNEL_INFO_VA);
  console_channel_base_va = (QWORD) (CONSOLE_CHANNEL_BASE_VA);

#ifdef DEBUG
  lk_print_xy(0, 10, "Console console_channel_info_va %q, pa %q  ", console_channel_info_va, get_pa(console_channel_info_va));
#endif
  //ckeck console proxy is running 
  g_console_magic = ( ((QWORD *) console_channel_info_va) + g_console_id );

#if 1
  while(*g_console_magic != (QWORD) (CONSOLE_MAGIC + g_console_id)) {
     __asm volatile ("pause" ::: "memory");
  }

  g_console_proxy_flag = TRUE;
#else
  if (*g_console_magic != (QWORD) (CONSOLE_MAGIC + g_console_id)) {
    g_console_proxy_flag = FALSE;
    lk_print("Console proxy(id:%d) is not running.\n", g_console_id);
    //return (FALSE);
  }
  else {
    g_console_proxy_flag = TRUE;
  }
#endif

  n_ipages =  CONSOLE_CHANNEL_CQ_ELEMENT_NUM * CQ_ELE_PAGE_NUM + 1;
  n_opages =  CONSOLE_CHANNEL_CQ_ELEMENT_NUM * CQ_ELE_PAGE_NUM + 1;

  for(console_channel_offset = g_console_id; console_channel_offset < (g_console_id + 1); console_channel_offset++) {
    // init console channel
    init_channel(&g_console_channels[g_console_id]);
    console_channel = &g_console_channels[g_console_id];

    // map icq of console channel
    icq_base_va = (QWORD) console_channel_base_va + console_channel_offset * (n_ipages + n_opages) * PAGE_SIZE_4K;
    console_channel->in = (struct circular_queue *) (icq_base_va);
    cq_init(console_channel->in, (n_ipages - 1) / CQ_ELE_PAGE_NUM);
    mutex_init(&console_channel->in->lock);

    // map ocq of console channel
    ocq_base_va = (QWORD) icq_base_va + (n_ipages * PAGE_SIZE_4K);
    console_channel->out = (struct circular_queue *) (ocq_base_va);
    cq_init(console_channel->out,(n_opages - 1) / CQ_ELE_PAGE_NUM);
    mutex_init(&console_channel->out->lock);
  }

  if(g_console_proxy_flag == TRUE) {
    lk_print("Console proxy(id:%d) is running. (%q).\n", g_console_id, *g_console_magic);
    return(TRUE);
  }
  else {
    return(FALSE);
  }
}


/**
 * @brief get console channel
 * @return success (pointer to channel), fail (NULL)
 */
channel_t *get_console_channel(void)
{
  return (&(g_console_channels[g_console_id]));
}
