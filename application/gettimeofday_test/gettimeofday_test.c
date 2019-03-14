#include "utility.h"
#include "syscall.h"

#include <sys/time.h>

extern int sys_gettimeofday(struct timeval *, void *);

#if 0
int main()
{
  struct az_timeval tv1,tv2 ;

  print_xy(0,0, "HELLO") ;

  az_gettimeofday(&tv1, NULL) ;

  print_xy(0,1, "HELLO2") ;

  delay(1) ;

  print_xy(0,2,"HELLO3") ;

  az_gettimeofday(&tv2, NULL) ;

  print_xy(0,3, "sec %q , msec %q ", tv2.tv_sec, tv2.tv_usec ) ;
  print_xy(0,4, "sec %q , msec %q ", tv1.tv_sec, tv1.tv_usec ) ;

  return 0;
}
#endif

int main()
{
  struct timeval tv1;

  sys_gettimeofday(&tv1, NULL) ;

  print_xy(0,1, "HELLO2") ;

  delay(1) ;

  print_xy(0,2,"HELLO3") ;

  print_xy(0,4, "sec %q , msec %q ", tv1.tv_sec, tv1.tv_usec ) ;

  return 0;
}

