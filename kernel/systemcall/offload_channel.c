#include "console.h"
#include "offload_channel.h"
#include "offload_memory_config.h"
#include "page.h"
//#include "thread.h"

//extern QWORD g_memory_start;
static QWORD g_config_shared_memory_va;

//channel_t *offload_channels = (channel_t *) va(OFFLOAD_CHANNEL_TABLE_PA);
static channel_t *offload_channels;

static int g_n_offload_channels;

void cq_init(struct circular_queue *cq, unsigned long size)
{
  cq->head = 0;
  cq->tail = 0;
  cq->size = size;
}

int cq_avail_data(struct circular_queue *cq)
{
  return (cq->head - cq->tail) & (cq->size - 1);
}

int cq_free_space(struct circular_queue *cq)
{
  return (cq->size - 1 - cq_avail_data(cq));
}

int cq_is_full(struct circular_queue *cq)
{
  return ((cq->tail + 1) % cq->size == cq->head);
}

int cq_is_empty(struct circular_queue *cq)
{
  return (cq->head == cq->tail);
}

// Call this function after confirming enough room is available
int cq_add_data(struct circular_queue *cq, char data)
{
  while (cq_free_space(cq) == 0);
  (cq->data + cq->head)->d[0] = data;
//    mbarrier();
  cq->head = (cq->head + 1) % cq->size;

  return 1;
}

// Call this function after confirming requested size of data is available
int cq_remove_data(struct circular_queue *cq, char *data)
{
  while (cq_avail_data(cq) == 0);
  *data = (cq->data + cq->tail)->d[0];
//    mbarrier();
  cq->tail = (cq->tail + 1) % cq->size;

  return 1;
}

/*
 * initialize nvme io offload
 */
void init_offload_channel()
{
	volatile QWORD offload_magic = 0;

	channel_t *cur_channel;
	int offload_channels_offset = 0;

	QWORD n_icq = 0;
	QWORD n_ocq = 0;
	//QWORD icq_base_pa = 0;
	//QWORD ocq_base_pa = 0;
	QWORD icq_base_va = 0;
	QWORD ocq_base_va = 0;
	QWORD offload_channel_info_va = 0;
	QWORD offload_channel_base_va = 0;
	
	g_config_shared_memory_va = (unsigned long) CONFIG_SHARED_MEMORY_VA; 
 
	offload_channels = (channel_t *) ((QWORD) g_config_shared_memory_va + (PAGE_SIZE_4K * 2));

	// set nvme io offload channel info va
	offload_channel_info_va = (QWORD) (g_config_shared_memory_va + (PAGE_SIZE_4K * 10));
	offload_channel_base_va = (QWORD) (g_config_shared_memory_va + (PAGE_SIZE_4K * 11));

	do {
		offload_magic = *((QWORD *) offload_channel_info_va);
	} while (offload_magic != OFFLOAD_MAGIC);

	g_n_offload_channels = * ((QWORD *)(offload_channel_info_va) + 1);
	n_icq = *((QWORD *)(offload_channel_info_va) + 2);
	n_ocq = *((QWORD *)(offload_channel_info_va) + 3);

	// initialize offload channel
	for(offload_channels_offset = 0; offload_channels_offset < g_n_offload_channels; offload_channels_offset++) {

		icq_base_va = (QWORD) offload_channel_base_va + offload_channels_offset * (n_icq + n_ocq) * PAGE_SIZE_4K;
		// map icq of ith channel
		cur_channel = &offload_channels[offload_channels_offset];

		cur_channel->in = (struct circular_queue *) (icq_base_va);

		// map ocq of ith channel
		ocq_base_va = (QWORD) icq_base_va + (n_icq * PAGE_SIZE_4K);
		cur_channel->out = (struct circular_queue *) (ocq_base_va);
	}
}

/*
 * get offload channel
 * channel: 0 ~ (OFFLOAD_MAX_CHANNEL-1)
 */
channel_t *get_offload_channel(int n_requested_channel)
{
  int offload_channels_offset = 0;

  offload_channels_offset = (n_requested_channel == -1) ? get_apic_id() : n_requested_channel;

  offload_channels_offset = (int) (offload_channels_offset % g_n_offload_channels);
  return (&(offload_channels[offload_channels_offset]));
}

#if 0
int get_iovec(BYTE *buf, unsigned long nbytes, struct iovec *pa_iov, int *iovcnt)
{
QWORD mytid = -1;
int as_id = -1;
int page_type;
int page_size;
QWORD va = 0;
int offset = 0;
int length_in_page = 0;
int i = 0;
long long int size = 0;
BYTE temp;

TCB *current;

    current = get_current(); 
    mytid = current->id;
	as_id = lk_thread_lookup_aid(mytid);

	va = (QWORD) buf;
	size = nbytes;

	offset = (va & PAGE_OFFSET_MASK);
	int loop = (offset + nbytes) / PAGE_SIZE_4K;
	if((offset + nbytes) % PAGE_SIZE_4K > 0) 
		loop++;
    QWORD va_tmp = va - offset;
	for(i = 0; i < loop; i++) {
		temp = *((BYTE *)(va_tmp + i * PAGE_SIZE_4K)); // check page exists 
	}

	va = (QWORD) buf;
	size = nbytes;
	i = 0;
	while(size > 0) { 
		//temp = *((BYTE *)va); // check page exists 
		pa_iov[i].iov_base = get_pa(as_id, (QWORD) va, &page_type);
		//print_xy(0, 20, "page type = %d", page_type);
		//page_size = (page_type == 1) ? PAGE_SIZE_4K : PAGE_SIZE_2M;
		page_size = PAGE_SIZE_4K;
		offset = (va & PAGE_OFFSET_MASK);
		length_in_page = page_size - offset;

		if(offset + size >= page_size) {
			pa_iov[i].iov_len = length_in_page;

			va = va + length_in_page;
			size = size - length_in_page;
		}
		else {
			pa_iov[i].iov_len = size;
			size = 0;
		}
		i++;
		if(i > MAX_IOV_SIZE) return 0;
	}

	//print_xy(0, 21, "iovcnt = %d", i);
	*iovcnt = i;

	return 1;
}

#endif
