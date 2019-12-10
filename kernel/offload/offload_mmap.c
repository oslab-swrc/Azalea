#include "console.h"
#include "offload_mmap.h"
#include "offload_memory_config.h"
#include "page.h"
#include "utility.h"
#include "multiprocessor.h"

int g_offload_initialization_flag;
static channel_t *g_offload_channels;
static int g_n_offload_channels;
static int g_n_nodes;
int g_node_id;


/**
 * @brief initialize io offload
 * @return success (TRUE), fail (FALSE)
 */
BOOL init_offload_channel()
{
	volatile QWORD offload_magic = 0;

	channel_t *cur_channel;
	int offload_channels_offset = 0;

	QWORD n_ipages = 0;
	QWORD n_opages = 0;
	QWORD icq_base_va = 0;
	QWORD ocq_base_va = 0;
	QWORD offload_channel_info_va = 0;
	QWORD offload_channel_base_va = 0;
	QWORD *p_node_id = NULL;
 
	g_offload_channels = (channel_t *) ((QWORD) OFFLOAD_CHANNEL_STRUCT_VA);

	// set io offload channel info va
	offload_channel_info_va = (QWORD) (OFFLOAD_CHANNEL_INFO_VA);
	offload_channel_base_va = (QWORD) (OFFLOAD_CHANNEL_BASE_VA);

	offload_magic = *((QWORD *) offload_channel_info_va);
	// check offload channel is initialized or not
	if (offload_magic != (QWORD) OFFLOAD_MAGIC) {
                g_offload_initialization_flag = FALSE;
                return (FALSE);
        }
        else {
                g_offload_initialization_flag = TRUE;
        }

	g_n_offload_channels = * ((QWORD *)(offload_channel_info_va) + 1);
	n_ipages = *((QWORD *)(offload_channel_info_va) + 2);
	n_opages =  *((QWORD *)(offload_channel_info_va) + 3);
	g_n_nodes = *((QWORD *)(offload_channel_info_va) + 4);
	p_node_id = (QWORD *)(offload_channel_info_va + sizeof(QWORD) * 5);
	g_node_id = (int) *p_node_id;
	(*p_node_id)++;

	// initialize offload channel
	//for(offload_channels_offset = 0; offload_channels_offset < g_n_offload_channels; offload_channels_offset++) {
	for(offload_channels_offset = g_node_id * (g_n_offload_channels / g_n_nodes); offload_channels_offset < (g_node_id + 1) * (g_n_offload_channels / g_n_nodes); offload_channels_offset++) {

		icq_base_va = (QWORD) offload_channel_base_va + offload_channels_offset * (n_ipages + n_opages) * PAGE_SIZE_4K;
		// map icq of ith channel
		cur_channel = &g_offload_channels[offload_channels_offset];

		cur_channel->in = (struct circular_queue *) (icq_base_va);
		if(g_node_id == 0) {
			//lock init
			mutex_init(&cur_channel->in->lock);
		}

		// map ocq of ith channel
		ocq_base_va = (QWORD) icq_base_va + (n_ipages * PAGE_SIZE_4K);
		cur_channel->out = (struct circular_queue *) (ocq_base_va);
		if(g_node_id == 0) {
			//lock init
			mutex_init(&cur_channel->out->lock);
		}
	}

	return(TRUE);
}

/**
 * @brief get offload channel
 * channel: 0 ~ (OFFLOAD_MAX_CHANNEL-1)
 * @param n_requested_channel channel number to use
 * @return success (pointer to channel), fail (NULL)
 */
channel_t *get_offload_channel(int n_requested_channel)
{
  int offload_channels_offset_in_node = 0;
  int offload_channels_size_per_node = 0;
  int offload_channels_index = 0;

  // offload channel is not initialized
  if(g_offload_initialization_flag == FALSE)
	return (NULL);

#if 0
  if(get_papic_id() >= g_n_offload_channels)
    return (&(g_offload_channels[get_apic_id()]));

#else
  if(g_n_nodes != 0) 
  	offload_channels_size_per_node = g_n_offload_channels / g_n_nodes;
  else {
  	lk_print_xy(0, 24, "Offload total node number is not set: node id %d", g_node_id);
	return (NULL);
  }

  offload_channels_offset_in_node = (n_requested_channel == -1) ? get_apic_id() : n_requested_channel;
  offload_channels_index = offload_channels_size_per_node * g_node_id + offload_channels_offset_in_node;

  return (&(g_offload_channels[offload_channels_index]));
#endif
}

