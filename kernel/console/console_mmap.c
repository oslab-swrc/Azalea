#include "offload_channel.h"
#include "console_memory_config.h"
#include "page.h"
#include "utility.h"

#include <sys/lock.h>
//#include "console.h"
//#include "map.h"
#include "memory.h"
#include "az_types.h"


//#include "offload_mmap.h"

#define PAGE_PTE_OFFSET_MASK    (PAGE_PTE_MASK + PAGE_OFFSET_MASK)

//extern QWORD g_memory_start;

int g_console_id;
static channel_t g_console_channel;

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
	//lk_print_xy(0, 9, "#ch %d, #ipage %d, #opage %d, #nodes %d, node id %d dis %q", CONSOLE_CHANNEL_NUM, n_ipages, n_opages, MAX_NODE_NUM, g_node_console_id, ocq_base_va - icq_base_va);
		//}

	return(TRUE);
}



/**
 * @brief get console channel
 * channel: 0 ~ (MAX_NODE_NUM-1)
 * @param n_requested_channel channel number to use
 * @return success (pointer to channel), fail (NULL)
 */
channel_t *get_console_channel(void)
{
  //lk_print("\nget_output_console_channe: %q, %d", (unsigned long) CONSOLE_CHANNEL_BASE_VA, g_node_console_id);
  return (&g_console_channel);
}



#if 0
/**
 * @brief Get physical address corresponding to virtial address
 * @param virtual_address virtual address
 * @return success physical address, fail FALSE
 */
unsigned long get_pa(QWORD virtual_address)
{
  PML4T_ENTRY *pml4t_entry = NULL;
  PDPT_ENTRY *pdpt_entry = NULL;
  PD_ENTRY *pd_entry = NULL;
  PT_ENTRY *pt_entry = NULL;
  QWORD pgd = 0;
  QWORD new_pt_page = 0;
  QWORD new_page = 0;
  int index = -1;

  // address  is NULL
  if(virtual_address == 0)
    return (0); 

  // identical mapping in high half memory
  if(virtual_address > CONFIG_HIGH_HALF_LIMIT) {
#ifdef DEBUG
  lk_print_xy(25, 1, "get_pa(ID) va:%Q pa:%Q", virtual_address, pa(virtual_address));
#endif
    return (pa(virtual_address)); 
  }

  spinlock_lock(&map_lock);

  //pgd = get_as(address_space_id)->pgd;
  pgd = CONFIG_KERNEL_PAGETABLE_ADDRESS;

  pml4t_entry = (PML4T_ENTRY *) va(pgd);

  index = (virtual_address & PAGE_PML4_MASK) >> PAGE_PML4_SHIFT;


  if (pml4t_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pml4t_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  // PDPT entry
  pdpt_entry = (PDPT_ENTRY *) va(pml4t_entry[index].entry & (~PAGE_ATTR_MASK));

  index = (virtual_address & PAGE_PDPT_MASK) >> PAGE_PDPT_SHIFT;
  if (pdpt_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pdpt_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  // PD entry
  pd_entry = (PD_ENTRY *) va(pdpt_entry[index].entry & (~PAGE_ATTR_MASK));

  index = (virtual_address & PAGE_PDE_MASK) >> PAGE_PDE_SHIFT;
  if (pd_entry[index].entry == 0) {
    new_pt_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_pt_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    set_page_entry_data(&pd_entry[index], pa(new_pt_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }
  else {
    // PAGE_SIZE_2M is used
    if ((pd_entry[index].entry & PAGE_FLAGS_PS) == PAGE_FLAGS_PS) {
      spinlock_unlock(&map_lock);
#ifdef DEBUG
      lk_print_xy(25, 2, "get_pa(2M) va:%Q pa:%Q", virtual_address, (pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
#endif
      return ((pd_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_PTE_OFFSET_MASK));
    }
  }

  // PT entry
  pt_entry = (PT_ENTRY *) va(pd_entry[index].entry & (~PAGE_ATTR_MASK));
  index = (virtual_address & PAGE_PTE_MASK) >> PAGE_PTE_SHIFT;

  if (pt_entry[index].entry == 0) {
    new_page = (QWORD) az_alloc(PAGE_SIZE_4K);

    if (new_page == (QWORD) NULL) {
      spinlock_unlock(&map_lock);
      return FALSE;
    }

    //lk_print_xy(25, 4, "alloced(4K) va:%Q pa:%Q", new_page, pa(new_page));
    set_page_entry_data(&pt_entry[index], pa(new_page), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US);
  }

  spinlock_unlock(&map_lock);

#ifdef DEBUG
  lk_print_xy(25, 3, "get_pa(4K) va:%Q pa:%Q", virtual_address, (pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
#endif
  return ((pt_entry[index].entry & (~PAGE_ATTR_MASK)) + (virtual_address & PAGE_OFFSET_MASK));
}
#endif

