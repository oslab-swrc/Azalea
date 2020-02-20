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

/**
 * Load the disk image to the unikernel with parameters
 *
 * param[0]-index, 
 * param[1]-cpu_start, param[2]-cpu_end, 
 * param[3]-memory_start, param[4]-memory_end
 */
int main(int argc, char *argv[] )
{
  int fd = -1, fd1 = -1;
  int ret = 0;
  char *buf = NULL;
  unsigned short param[CONFIG_PARAM_NUM] = {0, };
  unsigned int filesize = 0, readbytes  = 0;
  int i = 0;

  if (argc < 7)	{
    printf("[usage] :lkload <disk.img> [index] [CPU] [MEMORY]\n"); 
    return -1;
  }

  for (i=0; i<CONFIG_PARAM_NUM; i++)
    param[i] = atoi(argv[i+2]);

  // Open files and save into the buffer
  fd1 = open(argv[1], O_RDONLY);

  if (fd1 < 0) {
    printf("[%s] open failed\n", argv[1]);
    return -1;
  }

  filesize = lseek(fd1, 0, SEEK_END);

  buf = malloc(filesize);

  if (buf == NULL) {
    printf("memory allocation failed \n");
    close(fd1);
    return -1;
  }

  lseek(fd1, 0, SEEK_SET);

  readbytes = read(fd1, buf, filesize);

  if (readbytes != filesize) {
    printf("read failed from disk.img\n");
    free(buf);
    close(fd1);
    return -1;
  }

  close(fd1);
  printf("%s opened... [FILESIZE : %d]\n", argv[1], filesize);

  // Open the lk module
  fd = open("/dev/lk", O_RDONLY);

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
  printf("kernel image loading done...\n");

  close(fd);

  free(buf);

  return 0;
}
