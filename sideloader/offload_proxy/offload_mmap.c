#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "offload_mmap.h"
#include "offload_memory_config.h"

// all unikernels memory start address
unsigned long g_mmap_unikernels_mem_base_va = 0;

// offload channel info address
unsigned long g_offload_channels_info_va = 0;

/*
 * munmap_channels()
 */
int munmap_channels(channel_t *offload_channels, int n_offload_channels)
{
	int offload_channels_offset = 0;
	int err = 0;

	// munmap channel
	for(offload_channels_offset = 0; offload_channels_offset < n_offload_channels; offload_channels_offset++) 
	{
		if(munmap(offload_channels[offload_channels_offset].out_cq, offload_channels[offload_channels_offset].out_cq_len) < 0) {
			printf("munmap failed.\n");
			err++;
		}
		if(munmap(offload_channels[offload_channels_offset].in_cq, offload_channels[offload_channels_offset].in_cq_len) < 0) {
			printf("munmap failed.\n");
			err++;
		}
	}

	// munmap channel informaiton
	if(munmap((void *) g_offload_channels_info_va, (size_t) PAGE_SIZE_4K) < 0) {
		printf("munmap failed.\n");
		err++;
	}

	if(err) {
		return -1;
	}

	return 0;
}

/*
 * mmap_channels()
 */
int mmap_channels(channel_t *offload_channels, int n_offload_channels, int opages, int ipages)
{
	int offload_channels_offset = 0;

	int offload_fd = 0;

	unsigned long in_cq_base = 0;
	unsigned long in_cq_base_pa = 0;
	unsigned long in_cq_base_pa_len = 0;

	unsigned long out_cq_base = 0;
	unsigned long out_cq_base_pa = 0;
	unsigned long out_cq_base_pa_len = 0;

	unsigned long offload_channels_info_va = 0;
	//unsigned long offload_channels_info_pa = 0;
	unsigned long *offload_channels_info = NULL;

	offload_fd = open("/dev/offload", O_RDWR) ;
	if (offload_fd < 0)
	{
		printf("/dev/offload open error\n") ;
		return (0);
	}

	offload_channels_info_va = (unsigned long) mmap(NULL, (size_t) PAGE_SIZE_4K, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, (off_t) OFFLOAD_CHANNEL_INFO_PA);
	g_offload_channels_info_va = (unsigned long) offload_channels_info_va;

	// set channel information
	offload_channels_info = (unsigned long *) offload_channels_info_va;
	*(offload_channels_info++) = (unsigned long) 0;
	*(offload_channels_info++) = (unsigned long) n_offload_channels;
	*(offload_channels_info++) = (unsigned long) opages;
	*(offload_channels_info) = (unsigned long) ipages;

	for(offload_channels_offset = 0; offload_channels_offset < n_offload_channels; offload_channels_offset++) {

		// mmap ocq of ith channel
		out_cq_base_pa = (unsigned long) OFFLOAD_CHANNEL_BASE_PA + offload_channels_offset * (opages + ipages) * PAGE_SIZE_4K;
		out_cq_base_pa_len = (unsigned long) (opages * PAGE_SIZE_4K);
		out_cq_base = (unsigned long) mmap(NULL, out_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, out_cq_base_pa);
		if(out_cq_base == (unsigned long) MAP_FAILED)
		{
			printf("mmap failed.\n") ;
			munmap_channels(offload_channels, offload_channels_offset);
			close(offload_fd) ;
			return (0);
		}

		offload_channels[offload_channels_offset].out_cq = (struct circular_queue *)(out_cq_base);
		offload_channels[offload_channels_offset].out_cq_len = out_cq_base_pa_len;

		//init cq
		cq_init(offload_channels[offload_channels_offset].out_cq, (opages - 1) / CQ_ELE_PAGE_NUM);

		// mmap icq of ith channel
		in_cq_base_pa = (unsigned long) out_cq_base_pa + (opages * PAGE_SIZE_4K);
		in_cq_base_pa_len = (unsigned long) (ipages * PAGE_SIZE_4K);
		in_cq_base = (unsigned long) mmap(NULL, in_cq_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, in_cq_base_pa);
		if(in_cq_base == (unsigned long) MAP_FAILED)
		{
			printf("mmap failed.\n") ;
			munmap(offload_channels[offload_channels_offset].out_cq, offload_channels[offload_channels_offset].out_cq_len);
			munmap_channels(offload_channels, offload_channels_offset);
			close(offload_fd) ;
			return (0);
		}
		offload_channels[offload_channels_offset].in_cq = (struct circular_queue *)(in_cq_base);
		offload_channels[offload_channels_offset].in_cq_len = in_cq_base_pa_len;

		//init cq
		cq_init(offload_channels[offload_channels_offset].in_cq, (ipages - 1) / CQ_ELE_PAGE_NUM);
	}

	//set OFFLOAD_MAGIC
	offload_channels_info = (unsigned long *) offload_channels_info_va;
	*(offload_channels_info) = (unsigned long) OFFLOAD_MAGIC;

	close(offload_fd);

	return (1);
}


/*
 * mmap_unikernels_memory()
 */
int mmap_unikernels_memory()
{
	unsigned long kernels_mem_base_pa;
	unsigned long kernels_mem_base_pa_len;

	int offload_fd = 0;

	offload_fd = open("/dev/offload", O_RDWR | O_SYNC) ;
	if (offload_fd < 0)
	{
		printf("/dev/offload open error\n") ;
		return (1);
	}

	kernels_mem_base_pa = UNIKERNELS_MEM_BASE_PA; // 48G
	kernels_mem_base_pa_len = UNIKERNELS_MEM_SIZE; // 48G * 3

	g_mmap_unikernels_mem_base_va = (unsigned long) mmap(NULL, kernels_mem_base_pa_len, PROT_WRITE | PROT_READ, MAP_SHARED, offload_fd, kernels_mem_base_pa);

	if(g_mmap_unikernels_mem_base_va == (unsigned long) MAP_FAILED)
	{
		printf("mmap failed.\n") ;
		close(offload_fd) ;
		return (1);
	}
	printf("mmap = virtual address: 0x%lx physical address begin: 0x%lx end: 0x%lx\n", g_mmap_unikernels_mem_base_va, kernels_mem_base_pa, kernels_mem_base_pa + kernels_mem_base_pa_len) ;

	close(offload_fd) ;

	return (0);

}

/*
 * get_va()
 * get mmapped virtual address that matches physical address of unikernel
 */
unsigned long get_va(unsigned long pa)
{
        unsigned long offset = 0;

        if(pa == 0) {
                return (0);
        }

        offset = (unsigned long) pa - UNIKERNELS_MEM_BASE_PA;

        return (g_mmap_unikernels_mem_base_va + offset);
}



unsigned long get_pa_base() 
{
	return(UNIKERNELS_MEM_BASE_PA);
}

unsigned long get_va_base() 
{
	return (g_mmap_unikernels_mem_base_va);
}


