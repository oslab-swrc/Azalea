#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

  int fd = -1;
  int ret = -1;
  unsigned int phy = 0;

  if (argc < 2) {
    printf("off <acpi id>\n");
    return -1;
  }


  phy = atoi(argv[1]);

  printf("off: %d\n", phy);

  fd = open("/dev/lk", O_RDONLY);

  if (fd < 0) {
    printf("open error\n");
    return -1;
  }


  ret = ioctl(fd, 110, &phy);

  if (ret < 0)
    printf(" [ %d ] is already off\n", phy);

  close(fd);

  return 0;
}
