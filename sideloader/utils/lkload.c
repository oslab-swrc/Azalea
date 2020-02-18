#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../lk/lkernel.h"
#include "arch.h"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

/**
 * Load the disk image to the unikernel with parameters
 *
 * param[0]-index, 
 * param[1]-cpu_start, param[2]-cpu_end, 
 * param[3]-memory_start, param[4]-memory_end
 */
int main(int argc, char *argv[] )
{
  int fd = 0, fd1 = 0;
  int ret = 0;
  char *buf = NULL;
  char *addr = NULL ;
  unsigned short param[CONFIG_PARAM_NUM] = {0, };
  unsigned int filesize = 0, readbytes  = 0;
  int i = 0;

  unsigned short g_kernel32 = 0 ;
  unsigned short g_kernel64 = 0 ;
  unsigned short g_uapp = 0;
  unsigned short g_total = 0 ;

  short start_index = 0 ; 
  unsigned int core_start = 0, core_end = 0 ;
  unsigned long memory_start, memory_end, memory_start_addr, memory_shared_addr;

  if (argc < 7)	{
    printf("[usage] :lkload <disk.img> [index] [CPU] [MEMORY]\n"); 
    return -1;
  }

  for (i=0; i<CONFIG_PARAM_NUM; i++)
    param[i] = atoi(argv[i+2]);

  // Open files and save into the buffer
  fd1 = open(argv[1], O_RDONLY);

  if (fd1 < 0) {
    printf(RED "LK_LOAD: " RESET "[%s] open failed\n", argv[1]);
    return -1;
  }

  filesize = lseek(fd1, 0, SEEK_END);

  buf = malloc(filesize);

  if (buf == NULL) {
    printf(RED "LK_LOAD: " RESET "memory allocation failed \n");
    close(fd1);
    return -1;
  }

  lseek(fd1, 0, SEEK_SET);

  readbytes = read(fd1, buf, filesize);

  if (readbytes != filesize) {
    printf(RED "LK_LOAD: " RESET "read failed from disk.img\n");
    free(buf);
    close(fd1);
    return -1;
  }

  close(fd1);
  printf(BLU "LK_LOAD: " RESET "%s opened... [FILESIZE: %d]\n", argv[1], filesize);

  start_index = atoi(argv[2]) ;
  core_start = (atoi(argv[3]) == 0) ? CPUS_PER_NODE : atoi(argv[3]);
  core_end = 0;    // deprecated
  if (start_index != -1 ) {
     memory_start = UNIKERNEL_START + (start_index * MEMORYS_PER_NODE);   // memory_start
     memory_end = UNIKERNEL_START + ((start_index+1) * MEMORYS_PER_NODE);  // memory_end
  } else {
     memory_start = atoi(argv[5]) ;
     memory_end = atoi(argv[6]) ;
  }

  memory_start_addr = memory_start << 30 ;
  memory_shared_addr = ((unsigned long) (UNIKERNEL_START-SHARED_MEMORY_SIZE)) << 30;

  printf(BLU "LK_LOAD: " RESET "CPU list: %d / Memory: %ldG~%ldG\n", atoi(argv[3]),memory_start, memory_end) ;

  g_total = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+0)) ;
  g_kernel32 = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+2)) ;
  g_kernel64 = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+4)) ;
  g_uapp = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+6)) ;

  printf(BLU "LK_LOAD: " RESET "total: %d [bootloader:%d / kernel64:%d / uthread:%d]\n",
           g_total, g_kernel32, g_kernel64, g_total - g_kernel32 - g_kernel64);


  // Open the lk module
  fd = open("/dev/lk", O_RDWR);

  if (fd < 0) {
    printf(" /dev/lk open error\n");
    return -1;
  }

  // Send parameters
  ret = ioctl(fd, LK_PARAM, param);

  if (ret < 0) {
    printf("Sending parameters failed \n");
    close(fd);
    free(buf);
    return -1;
  }

  // Set the size of total, kernel32, kernel64
  ret = ioctl(fd, LK_IMG_SIZE, buf+TOTAL_COUNT_OFFSET);  // 0x83 : address of total in disk.img

  if (ret < 0) {
    printf("setting kernel image size failed \n"); 
    close(fd);
    free(buf);
    return -1;
  }

  // load kernel image 
  ret = ioctl(fd, LK_LOADING, buf);
  if (ret < 0) {
    printf("kernel image loading failed \n");
    close(fd);
    free(buf);
    return -1;
  }

  printf(BLU "LK_LOAD: " RESET "copy trampoline and make inital pagetable\n") ;

  addr = mmap(NULL, (g_total-g_kernel32)*SECTOR , PROT_WRITE | PROT_READ, MAP_SHARED, fd, memory_start_addr + KERNEL_ADDR );
 
  if ( addr == NULL )
  {
	  printf(RED "LK_LOAD: " RESET "failed to load kernel-lib\n") ;
          close(fd) ;
	  free(buf) ; 
	  return -1 ;

  }
  memcpy(addr, buf + (g_kernel32 * SECTOR) , g_kernel64*SECTOR ) ;
  munmap(addr, g_kernel64*SECTOR) ;

  addr =  mmap(NULL, (g_total-g_kernel32-g_kernel64)*SECTOR, PROT_WRITE | PROT_READ, MAP_SHARED, fd, memory_start_addr + APP_ADDR );

  if (addr == NULL )
  {	
	  printf(RED "LK_LOAD: " RESET "failed to load apps\n") ;
	  close(fd) ;
	  free(buf) ;
	  return -1 ;

  }

  memcpy(addr, buf+(g_kernel32*SECTOR)+(g_kernel64 * SECTOR), (g_total-g_kernel32-g_kernel64) * SECTOR);

  printf(BLU "LK_LOAD: " RESET "Application copied from [%d] size:[%d] bytes to %p\n",
           (g_kernel32+g_kernel64) * SECTOR, (g_total-g_kernel32-g_kernel64) * SECTOR, addr );
  
  munmap(addr, (g_total - g_kernel32 - g_kernel64) * SECTOR ) ;

  close(fd);
  free(buf);

  return 0;
}
