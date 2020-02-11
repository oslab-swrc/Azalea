#include <sys/lock.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "console.h"
#include "page.h"
#include "thread.h"
#include "timer.h"

#include "offload_page.h"
#include "offload_channel.h"

#include "console_function.h"
#include "console_mmap.h"
#include "console_message.h"

extern BOOL g_console_proxy_flag;
extern QWORD *g_console_magic;

/**
 * @brief console print for boot screen
 * @param void
 * @return none
 */
int cs_boot_msg_print(int yloc) {
int x = 0, y = 0;
char *screen = NULL;
char line[CONSOLE_WIDTH+3];

  if(g_console_proxy_flag == FALSE)
    return (-1);

  screen = (char *) get_vcon_addr();
  for (y = 0; y < yloc; y++) {
    for (x = 0; x < CONSOLE_WIDTH; x++) {
      line[x] = *(screen + y*CONSOLE_WIDTH + x);
    }
    line[CONSOLE_WIDTH] = '\n';
    line[CONSOLE_WIDTH+1] = '\r';
    line[CONSOLE_WIDTH+2] = '\0';
    cs_write(1, line, lk_strlen(line));
  }

  return (0);
}


/**
 * @brief console write call
 * @param fd file descriptor
 * @param buf buffer
 * @param count count bytes to write
 * @return success (0), fail (-1)
 */
ssize_t cs_write(int fd, void *buf, size_t count)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

ssize_t ssret = 0;

struct iovec iov[MAX_IOV_NUM];
int iovcnt = 0;

  if(g_console_proxy_flag == FALSE)
    return (-1);

  if(count <= 0) return (-1);

  ch = get_console_channel();
  if(ch == NULL) return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  if(current == NULL) {
    mytid = -1;
  }
  else {
    mytid = current->id;
  }

  get_iovec(buf, count, iov, &iovcnt);
  send_console_message(ocq, mytid, CONSOLE_WRITE, fd, get_pa((QWORD) iov), iovcnt, 0, 0, 0);
  ssret= receive_console_message(icq, mytid, CONSOLE_WRITE);

  return (ssret);
}

/**
 * @brief console printf call
 * @param const char *parameter, ...
 * @return success (0), fail (-1)
 */
int cs_printf(const char *parameter, ...)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

struct iovec iov[MAX_IOV_NUM];
int iovcnt = 0;

TCB *current = NULL;
QWORD mytid = -1;

int iret = 0;

va_list ap;
char buf[1024];
int len = 0;

  if(g_console_proxy_flag == FALSE)
    return (-1);

  lk_memset(buf, 0, sizeof(char) * 1024);

  va_start(ap, parameter);
  len = lk_vsprintf(buf, parameter, ap);
  va_end(ap);

  ch = get_console_channel();
  if(ch == NULL) return (-1);

  ocq = ch->out;
  icq = ch->in;

  current = get_current();
  if(current == NULL) {
    mytid = -1;
  }
  else {
    mytid = current->id;
  }

  get_iovec(buf, len, iov, &iovcnt);
  send_console_message(ocq, mytid, CONSOLE_PRINT, get_pa((QWORD) iov), iovcnt, 0, 0, 0, 0);
  iret = receive_console_message(icq, mytid, CONSOLE_PRINT);

  return (iret);
}


/**
 * @brief console print call
 * @param buf buffer
 * @return success (0), fail (-1)
 */
int cs_puts(void *buf)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

size_t len = 0;
int iret = 0;

struct iovec iov[MAX_IOV_NUM];
int iovcnt = 0;

  if(g_console_proxy_flag == FALSE)
    return (-1);

  len = lk_strlen(buf);

  if(len <= 0) return (-1);

  ch = get_console_channel();
  if(ch == NULL) return (-1);

  ocq = ch->out;
  icq = ch->in;

  current = get_current();
  if(current == NULL) {
    mytid = -1;
  }
  else {
    mytid = current->id;
  }

  get_iovec(buf, len, iov, &iovcnt);
  send_console_message(ocq, mytid, CONSOLE_PRINT, get_pa((QWORD) iov), iovcnt, 0, 0, 0, 0);
  iret = receive_console_message(icq, mytid, CONSOLE_PRINT);

  return (iret);
}

/**
 * @brief console exit call
 * @param 
 * @return success (0), fail (-1)
 */
int cs_exit(void)
{
channel_t *ch = NULL;
struct circular_queue *icq = NULL;
struct circular_queue *ocq = NULL;

TCB *current = NULL;
QWORD mytid = -1;

int iret = 0;

  if(g_console_proxy_flag == FALSE)
    return (-1);

  ch = get_console_channel();
  if(ch == NULL) return (-1);

  ocq = ch->out;
  icq = ch->in;

  current = get_current();
  if(current == NULL) {
    mytid = -1;
  }
  else {
    mytid = current->id;
  }

  send_console_message(ocq, mytid, CONSOLE_EXIT, 0, 0, 0, 0, 0, 0);
  iret = receive_console_message(icq, mytid, CONSOLE_EXIT);

  if(g_console_magic != NULL) {
    *g_console_magic = 0;
  }

  return (iret);
}
