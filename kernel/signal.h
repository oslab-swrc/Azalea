#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <sys/lock.h>

#include "az_types.h"
#include "list.h"
#include "sync.h"

#define MAX_SIGNALS   (32)
#define SIGNAL_IRQ    (50)

typedef void (*signal_handler_t)(int);

// This is used in deqeue.h (HACK)
typedef struct _sig {
    tid_t dest;
    int signum;
    struct dl_list sig_link;
} signal_t;

struct signal_list {
  spinlock_t lock;
  QWORD count;
  struct dl_list sig_list;
};

// Functions
void signal_init(void);
int sys_kill(tid_t dest, int signum);
int sys_signal(signal_handler_t handler);
void do_signal(signal_handler_t handler, tid_t tid);

#endif /* __SIGNAL_H__ */
