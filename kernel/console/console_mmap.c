#include "offload_channel.h"
#include "console_memory_config.h"
#include "page.h"
#include "utility.h"

#include <sys/lock.h>
#include "memory.h"
#include "az_types.h"

#define PAGE_PTE_OFFSET_MASK    (PAGE_PTE_MASK + PAGE_OFFSET_MASK)


int g_console_id;
static channel_t g_console_channel;

/**
 * @brief get console id 
 * @return console id
 */
int get_console_id(void) 
{
	int current_memory_start = 0;
	int global_memory_start = 0;
	int memory_per_node = 0;

	current_memory_start = *(QWORD*) (CONFIG_MEM_START + CONFIG_PAGE_OFFSET);
	global_memory_start =  (int) UNIKERNEL_START;
	memory_per_node =  (int) MEMORYS_PER_NODE;
	g_console_id =  (int) ((current_memory_start - global_memory_start) / memory_per_node);

	return (g_console_id);
}


/**
 * @brief initialize console offload
 * @return success (TRUE), fail (FALSE)
 */
BOOL init_console_channel(void)
{
	QWORD n_ipages = 0;
	QWORD n_opages = 0;
	QWORD icq_base_va = 0;
	QWORD ocq_base_va = 0;
	QWORD console_channel_base_va = 0;

	// get console id
	get_console_id();
 
	// initialize console channel
  	init_channel(&g_console_channel);

	console_channel_base_va = (QWORD) (CONSOLE_CHANNEL_BASE_VA);

	n_ipages =  CONSOLE_CHANNEL_CQ_ELEMENT_NUM * CQ_ELE_PAGE_NUM + 1;
	n_opages =  CONSOLE_CHANNEL_CQ_ELEMENT_NUM * CQ_ELE_PAGE_NUM + 1;
 
	// map icq of ith channel
    icq_base_va = (QWORD) console_channel_base_va + g_console_id * (n_ipages + n_opages) * PAGE_SIZE_4K;
  	g_console_channel.in = (struct circular_queue *) (icq_base_va);
  	cq_init(g_console_channel.in, (n_ipages - 1) / CQ_ELE_PAGE_NUM);
  	mutex_init(&g_console_channel.in->lock);
  
  	// map ocq of ith channel
  	ocq_base_va = (QWORD) icq_base_va + (n_ipages * PAGE_SIZE_4K);
  	g_console_channel.out = (struct circular_queue *) (ocq_base_va);
  	cq_init(g_console_channel.out, (n_opages - 1) / CQ_ELE_PAGE_NUM);
  	mutex_init(&g_console_channel.out->lock);

	return(TRUE);
}


/**
 * @brief get console channel
 * @return success (pointer to channel), fail (NULL)
 */
channel_t *get_console_channel(void)
{
  return (&g_console_channel);
}
