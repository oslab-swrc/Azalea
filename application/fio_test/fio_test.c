#include "syscall.h"
#include "utility.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE (1024)

int main(void)
{
  char rfilename[128];
  char wfilename[128];
  int rfd;
  int wfd;
  int oflag = 4;//O_RDWR;
//  int mode = 0644;
  unsigned char buffer[BUF_SIZE];
  int count;
  int wcount;
  long total = 0;

  strcpy(rfilename, "/tmp/test.pgm");
  strcpy(wfilename, "/tmp/test_2.pgm");

  rfd = open(rfilename, oflag, 0644);
  wfd = sys_creat(wfilename, S_IRUSR | S_IWUSR);
  print_xy(0, 18, "18-----read fd %d", rfd);
  print_xy(0, 19, "19-----write fd %d", wfd);

#if 1
  while((count = read(rfd, buffer, BUF_SIZE)) > 0) {
    print_xy(0, 20, "20-----read count %d", count);
    total = total + count;
    wcount = write(wfd, buffer, count);
    print_xy(0, 21, "21-----write count %d", wcount);
  }
  print_xy(0, 22, "22-----total count %d", total);
#endif

  close(rfd);
  close(wfd);

  return 0;
}

