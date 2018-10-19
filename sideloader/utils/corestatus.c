#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char g_screen[256] ;

int main()
{
    int fd = 0;
    int ret = 0;

    memset(g_screen, 0, sizeof(g_screen));

    fd = open("/dev/lk", O_RDONLY) ;

    if ( fd < 0 )
    {
	printf("open error\n") ;
	return -1;
    }

    ret = ioctl(fd, 25, g_screen) ;

    if (ret != 0 ) return -1 ;

    g_screen[220] = '\0' ;

    printf("%s", g_screen) ;

    close(fd) ;

    return 0;
}
