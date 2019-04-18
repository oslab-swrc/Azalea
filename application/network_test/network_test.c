#include "syscall.h"
#include "utility.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef COUNT
#define COUNT_START static int aim_iteration_test_count = 0, caim_iteration_test_count = 0;
#define COUNT_ZERO aim_iteration_test_count = 0; caim_iteration_test_count = 0
#define COUNT_BUMP  { aim_iteration_test_count++; }
#define COUNT_END(a) if (caim_iteration_test_count++ == 0) printf("Count = %d for test %s in file %s at line %d\n", aim_iteration_test_count, a, __FILE__, __LINE__);
#else
#define COUNT_START
#define COUNT_BUMP
#define COUNT_ZERO
#define COUNT_END(a)
#endif

#define Members(x)      (sizeof(x)/sizeof(x[0]))

//#define FATAL(a) {fprintf(stderr,"\nFatal error %d at line %d of file %s: %s -- ",errno,__LINE__,__FILE__,a); perror(""); return(-1); }
#define FATAL(a) {print("Fatal error"); return(-1); }
#define MAXBLK (2048)
#define dump_socket(a,b)

static int count = 0;
static int sizes[] = {
    1, 3, 5, 7, 16, 32, 64, 512, 1024, 2048,    /* misc. sizes */
    1, 3, 5, 7, 16, 32, 64, 512, 1024, 2048,
    32, 32, 32, 32, 32, 32,     /* x windows mostly... */
    512, 512, 512, 512, 512,    /* DBMS's mostly */
};

struct hostent *hp;
char myname[1024];
struct sockaddr_in rd_in, wr_in;

int readn(int fd, char *buf, int size)
{
  int count, total, result;

  count = size + 2;       /* maximum iteration count */
  total = size;           /* initial amount to read */
  while (total > 0) {     /* while not done */
//  errno = 0;      /* clear errors */
    result = read(fd, buf, total);  /* read some */
    if (result < 0) {
//    if (errno == EINTR)
//      continue;   /* try again if interrupted */
      return result;  /* return errors here */
    }
    total -= result;    /* else reduce total */
    buf += result;      /* update pointer */
    if (--count <= 0) {
//    fprintf(stderr,
//      "\nMaximum iterations exceeded in readn(%d, %#x, %d)",
//      fd, (unsigned)buf, size);
      return (-1);
    }
  }               /* and loop */

  return (size - total);      /* calculate # bytes read */
}

int writen(int fd, char *buf, int size)
{
  int count, total, result;

  count = size + 2;       /* initialize max count */
  total = size;           /* total */
  while (total > 0) {     /* while not done */
//  errno = 0;      /* clear error number */
    result = write(fd, buf, total); /* write some */
    if (result < 0) {   /* handle unusual case */
//    if (errno == EINTR)
//      continue;   /* if interrupted, loop */
      return result;  /* return errors here */
    }
    total -= result;    /* else reduce total */
    buf += result;      /* update pointer */
    if (--count <= 0) { /* handle too many loops */
//    fprintf(stderr,
//      "\nMaximum iterations exceeded in writen(%d, %#x, %d)",
//      fd, (unsigned)buf, size);
      return (-1);
    }
  }               /* and loop */

  return (size - total);      /* calculate # bytes read */
}


/* the actual test */
int read_write_close(int loops, char *title, int rd_fd, int wr_fd)
{
  char buffer[MAXBLK];        /* input and output buffer */
  int
  size,                       /* transaction size */
  result;                     /* status result */

  COUNT_ZERO;                 /* clear the count */
  while (--loops > 0) {
    size = sizes[count++ % Members(sizes)]; /* get next size */
    result = writen(wr_fd, buffer, size);   /* write this much */
    if (result != size) {
//    fprintf(stderr, "Write returned %d -- ", result);
//      perror("pipe write");
      print("Write error\n");
      return (-1);
    }
    result = readn(rd_fd, buffer, size);    /* read this much */
    if (result != size) {
//    fprintf(stderr, "Read returned %d -- ", result);
//      perror("pipe read");
      print("Read error\n");
      return (-1);
    }
    COUNT_BUMP;
  }
  close(rd_fd);
  close(wr_fd);

  return (0);
}

int tcp_test(char *argv)
{
  int status, wr, rd, length, new;
  int i64 = 0;

  /*
   * Step 1: create write socket and everything
   */
  status = wr = sys_socket(AF_INET, SOCK_STREAM, 0);  /* create write socket */
  if (status < 0) {
//      FATAL("Creating write stream socket");  /* error test */
    print("Creating write stream socket failed");
    exit(-1);
  }
  print("Step 1: Socket success: %d\n", wr);

  /*
   * Step 2: initialize socket structure and then bind it
   */
  memset(&wr_in, 0, sizeof (wr_in));  /* clear it to zeros */
  wr_in.sin_family = AF_INET;         /* set family of socket */
  memcpy((void *)&wr_in.sin_addr.s_addr, (void *)hp->h_addr, hp->h_length);   /* ignore addresses */
  wr_in.sin_port = 0;                 /* set write port (make kernel choose) */
  status = sys_bind(wr, (struct sockaddr *)&wr_in, sizeof (wr_in));   /* do the bind */
  if (status < 0)
    FATAL("bind on write failed");    /* error test */
  print("Step 2: Bind success: %d, %d\n", status, wr_in.sin_family);

  /*
   * Step 3: update info
   */
  length = sizeof (wr_in);            /* get length */
  status = sys_getsockname(wr, (struct sockaddr *)&wr_in, (socklen_t *)&length);   /* get socket info */
  if (status < 0)
    FATAL("getsockname failed");      /* if error */
  print("Step 3: getsockname success: %d, %d\n", status, wr_in.sin_family);

#if 0
  /*
   * tell it not to delay sending data
   */
#ifdef TCP_NODELAY
  result = 1;         /* enable the option */
  status = setsockopt(wr, IPPROTO_TCP, TCP_NODELAY, (char *)&result, sizeof (result));    /* set it */
  if (status < 0)
    FATAL("setsockopt()");  /* die here if error */
  length = sizeof (result);   /* establish size */
  result = 0;         /* clear the result */
  status = getsockopt(wr, IPPROTO_TCP, TCP_NODELAY, (char *)&result, &length);    /* get it */
  if (status < 0)
    FATAL("getsockopt()");  /* die here if error */
#endif
#endif

  /*
   * Step 4: Allow connections
   */
  sys_listen(wr, 5);          /* listen for connections */
  print("Step 4: listen success\n");

  /*
   * Step 5: create read socket
   */
  status = rd = sys_socket(AF_INET, SOCK_STREAM, 0);  /* create read socket */
  if (status < 0)
    FATAL("Creating read stream socket");   /* error check */
  print("Step 5: socket success\n");

  /*
   * Step 6: connect to write socket
   */
  dump_socket("Write just before connect()", wr_in);
  status = sys_connect(rd, (struct sockaddr *)&wr_in, sizeof (wr_in));    /* do the connect */
  if (status < 0)
    FATAL("Connect failure");   /* error check */
  print("Step 6: connect success\n");

  /*
   * Step 7: allow connections
   */
    retry:                /* add retry 10/17/95 1.1  */
  new = status = sys_accept(wr, 0, 0);    /* do the accept */
//  if (status == -1 && errno == EINTR)
  if (status == -1)
    goto retry;     /* error check */
  print("Step 7: accept success\n");

  /*
   * Step 8: update information
   */
  length = sizeof (wr_in);    /* get length */
  status = sys_getsockname(rd, (struct sockaddr *)&rd_in, (socklen_t *) &length);   /* get socket info */
  if (status < 0)
    FATAL("getsockname()"); /* check for error */
  print("Step 8: getsockname success\n");

  /*
   * Step 9: go for it
   */
  status = read_write_close(i64, "TCP/IP", rd, new);  /* do the test */
  if (status < 0)
    FATAL("read_write_close()");    /* check for error */
  print("Step 9: read_write_close success\n");

  close(wr);

  return 0;
}

int main(void)
{
  char* param = "90" ;
  int status ;

  status = sys_gethostname(myname, sizeof (myname));  /* get machine name */
  if (status < 0) {               /* handle errors */
    print("gethostname error\n");
    exit(-1);
  }

  hp = sys_gethostbyname(myname);     /* what is my address? */
  if (hp == NULL) {               /* handle errors */
    print("gethostbyname error\n");
    exit(-1);
  }

  print("gethostname gethostbyname completed\n");

  tcp_test(param);

  print_xy(0, 0, "#################### NETWORK TEST DONE #########################");

  return 0;
}
