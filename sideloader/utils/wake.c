// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  int fd = -1;
  int ret = 0;
  unsigned int phy = 0;

  if (argc < 2) {
    printf("Wake <acpi id>\n");
    return -1;
  }

  phy = atoi(argv[1]);

//  printf("Wake on : %d\n", phy);

  fd = open("/dev/lk", O_RDONLY);

  if (fd < 0) {
    printf("/dev/lk open error\n");
    return -1;
  }


  ret = ioctl(fd, 100, &phy);
  if (ret < 0)
    printf(" [ %d ] is already on\n", phy);

  close(fd);

  return 0;
}
