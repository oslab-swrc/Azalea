#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../lk/lkernel.h"
#include "../lk/cmds.h"
#include "arch.h"

#define MAX_X		80

char *g_vcon;
char *g_screen ;

/**
 * Print the console
 *  - input: the number of console (0~MAX_UNIKERNEL);
 */
int main(int argc, char *argv[])
{
  int fd = -1;
  char Achar  = 0;
  int ret = 0;
  int loc = 0;
  int i = 0;

  // if the number is now set, set the default number (0) 
  loc = (argc < 2) ? 0 : atoi(argv[1]);

  // Print the console
  g_vcon = malloc(sizeof(PAGE_4K * MAX_UNIKERNEL));

  if ( g_vcon == NULL )
	  return -1 ;

  g_screen = g_vcon + (PAGE_4K*loc);

  // 
  fd = open("/dev/lk", O_RDONLY);

  if (fd < 0) {
    printf("open /dev/cmds error\n");
	return -1;
  }

  while(1) {
    ret = ioctl(fd, LK_CMD_CONSOLE, g_vcon);

    if (ret != 0) {
      sleep(5); 
      continue;
    }

	for (i=1; i<=25; i++) {
      Achar = g_screen[MAX_X*i] ;
      g_screen[MAX_X*i]='\0' ;
      printf("%s\n", g_screen+((i-1)*MAX_X));
      g_screen[MAX_X*i] = Achar ;
    }

    sleep(1);
  }

  close(fd) ;
  return 0;
}
