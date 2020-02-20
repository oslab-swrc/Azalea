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

/**
 * Print the log messages
 */
int main(int argc, char *argv[])
{
  int fd=-1, ret=0;
  int i;
  int len = (MAX_LOG_COUNT+1) * LOG_LENGTH;
  char *plog;

  plog = malloc(len);

  if ( plog == NULL )
	return -1 ;
  
  fd = open("/dev/lk", O_RDONLY);

  if ( fd >= 0 ) 
  {
  	ret = ioctl(fd, LK_CMD_LOG, plog);

  	for (i=0; i<len; i++) 
    		printf("%c", plog[i]);

  	close(fd);
	free(plog) ;
	return 0 ;
  }
  else {
	free(plog) ;
	return -1 ;
  }
}
