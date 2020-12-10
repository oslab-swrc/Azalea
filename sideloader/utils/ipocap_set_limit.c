#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "arch.h"
#include "../lk/stat.h"

int main(int argc, char *argv[])
{
  int fd = -1;
  int ret = 0;
  int plimit;

  if (argc < 2) {
    printf("ipocap_limit_set [power_limit(w)]\n");
    return -1;
  }

  plimit = atoi(argv[1]);

  fd = open("/dev/lk", O_RDONLY);

  if (fd < 0) {
    printf("/dev/lk open error\n");
    return -1;
  }


  ret = ioctl(fd, LK_CMD_IPOCAP, &plimit);
  if ( ret != 0 ) return -1 ;

  close(fd);

  return 0;
}
