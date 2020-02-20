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


/**
 * @brief Send message to kernel for printing in the kernel log
 * @param fd - ioctl descriptor
 * @param msg - message want to send
 * @return @return success (0), fail (-1)
 */
int print_kmsg(int fd, char *msg)
{
  int ret = 0;

  ret = ioctl(fd, AZ_PRINT_MSG, msg); 
  if (ret < 0) {
    printf("LK_LOAD: Print msg failed \n");
    return -1;
  }

  return 0;
}

/**
 * @brief Load the disk image to the unikernel with parameters
 * @param [0]-index, 
 * @param [1]-cpu_start, [2]-cpu_end, 
 * @prarm [3]-memory_start, [4]-memory_end
 */
int main(int argc, char *argv[] )
{

  int fd = -1, fd_lk = -1;

  int ret = 0;
  char *buf = NULL;
  char *addr = NULL ;
  char kmsg[256] = {0, };
  unsigned short param[CONFIG_PARAM_NUM] = {0, };
  unsigned int filesize = 0, readbytes  = 0;
  int i = 0;

  unsigned short g_kernel32 = 0;
  unsigned short g_kernel64 = 0;
  unsigned short g_total = 0;

  unsigned long memory_start_addr = 0;

  // Check parameters
  if (argc < 7)	{
    printf("[usage] :lkload <disk.img> [index] [CPU] [MEMORY]\n"); 
    return -1;
  }

  for (i=0; i<CONFIG_PARAM_NUM-2; i++)
    param[i] = atoi(argv[i+2]);

  // Open image file
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("LK_LOAD: [%s] open failed\n", argv[1]);
    return -1;
  }

  // Open the lk module
  fd_lk = open("/dev/lk", O_RDWR);
  if (fd_lk < 0) {
    printf("LK_LOAD: /dev/lk open error\n");
    return -1;
  }

  // Save image file to buffer
  filesize = lseek(fd, 0, SEEK_END);

  buf = malloc(filesize);
  if (buf == NULL) {
    print_kmsg(fd_lk, "LK_LOAD: memory allocation failed \n");
    close(fd);
    close(fd_lk);
    return -1;
  }

  lseek(fd, 0, SEEK_SET);

  readbytes = read(fd, buf, filesize);
  if (readbytes != filesize) {
    print_kmsg(fd_lk, "LK_LOAD: read failed from disk.img\n");
    free(buf);
    close(fd);
    close(fd_lk);
    return -1;
  }

  close(fd);

  // Get size metadata from image
  g_total = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+0)) ;
  g_kernel32 = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+2)) ;
  g_kernel64 = *((unsigned short *)(buf+TOTAL_COUNT_OFFSET+4)) ;

  param[CONFIG_PARAM_NUM-2] = g_total;
  param[CONFIG_PARAM_NUM-1] = g_kernel32;

  sprintf(kmsg, "LK_LOAD: total: %d [bootloader:%d / kernel64:%d / uthread:%d]\n",
           g_total, g_kernel32, g_kernel64, g_total - g_kernel32 - g_kernel64);
  print_kmsg(fd_lk, kmsg);


  // AZ_PARAM - send parameters to the kernel
  ret = ioctl(fd_lk, AZ_PARAM, param);
  if (ret < 0) {
    print_kmsg(fd_lk, "LK_LOAD: Sending parameters failed.\n");
    close(fd_lk);
    free(buf);
    return -1;
  }
  print_kmsg(fd_lk, "LK_LOAD: AZ_PARAM success.\n") ;

  // AZ_LOADING - send image to the kernel
  ret = ioctl(fd_lk, AZ_LOADING, buf);
  if (ret < 0) {
    print_kmsg(fd_lk, "LK_LOAD: Sending parameters failed.\n");
    close(fd_lk);
    free(buf);
    return -1;
  }
  print_kmsg(fd_lk, "LK_LOAD: AZ_LOADING success.\n") ;

  // AZ_GET_MEM_ADDR - get memory start addre from the kernel
  ret = ioctl(fd_lk, AZ_GET_MEM_ADDR, &memory_start_addr); 
  if (ret < 0) {
    print_kmsg(fd_lk, "LK_LOAD: Sending parameters failed.\n");
    close(fd_lk);
    free(buf);
    return -1;
  }
  print_kmsg(fd_lk, "LK_LOAD: AZ_GET_MEM_ADDR success.\n");

 // Copy Kernel into the memory
  addr = mmap(NULL, (g_total-g_kernel32)*SECTOR , PROT_WRITE | PROT_READ, MAP_SHARED, fd_lk, memory_start_addr + KERNEL_ADDR);
  if (addr == NULL) {
    print_kmsg(fd_lk, "LK_LOAD: failed to load kernel-lib\n");
    close(fd_lk);
    free(buf); 
    return -1;
  }
  memcpy(addr, buf + (g_kernel32 * SECTOR), g_kernel64 * SECTOR);
  munmap(addr, g_kernel64 * SECTOR);

  sprintf(kmsg, "LK_LOAD: Kernel copied from [%d] size [%d] bytes copied %p\n", (g_kernel32) * SECTOR, (g_total - g_kernel32) * SECTOR, addr);
  print_kmsg(fd_lk, kmsg);

  // Copy User application into the memory
  addr =  mmap(NULL, (g_total-g_kernel32-g_kernel64)*SECTOR, PROT_WRITE | PROT_READ, MAP_SHARED, fd_lk, memory_start_addr + APP_ADDR);
  if (addr == NULL) {
    print_kmsg(fd_lk, "LK_LOAD: failed to load apps.\n");
    close(fd_lk);
    free(buf);
    return -1;
  }
  memcpy(addr, buf+(g_kernel32*SECTOR)+(g_kernel64 * SECTOR), (g_total-g_kernel32-g_kernel64) * SECTOR);
  munmap(addr, (g_total - g_kernel32 - g_kernel64) * SECTOR);

  sprintf(kmsg, "LK_LOAD: Application copied from [%d] size:[%d] bytes to %p\n",
           (g_kernel32+g_kernel64) * SECTOR, (g_total-g_kernel32-g_kernel64) * SECTOR, addr );
  print_kmsg(fd_lk, kmsg);

  sprintf(kmsg, "LK_LOAD: %s opened... [FILESIZE: %d]\n", argv[1], filesize);
  print_kmsg(fd_lk, kmsg);

  close(fd_lk);
  free(buf);

  return 0;
}
