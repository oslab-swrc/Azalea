#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "systemcalllist.h"
#include "az_types.h"
#include "common.h"

// source: linux kernel 2.6.0 source tree/includeasm-x86-64/unistd.h

#define __syscall "syscall"

#define __syscall_clobber "r11", "rcx", "memory"

#define __syscall_return(type, res) \
do { \
  return (type) (res); \
} while (0) 

#define _syscall_null(type, name) \
type name(void) \
{\
  return 0; \
}

#define _syscall0(type, name) \
type name(void) \
{\
  QWORD __res; \
  __asm__ volatile (__syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name) \
      : __syscall_clobber ); \
  __syscall_return (type, __res); \
}

#define _syscall1(type, name, type1, arg1) \
type name(type1 arg1) \
{\
  QWORD __res; \
  __asm__ volatile (__syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)) \
      : __syscall_clobber ); \
  __syscall_return (type, __res); \
}

#define _syscall2(type, name, type1, arg1, type2, arg2) \
type name(type1 arg1, type2 arg2) \
{\
  QWORD __res; \
  __asm__ volatile (__syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)), "S" ((QWORD)(arg2)) \
      : __syscall_clobber ); \
  __syscall_return (type, __res); \
}

#define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3) \
type name(type1 arg1, type2 arg2, type3 arg3) \
{\
  QWORD __res; \
  __asm__ volatile (__syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)), "S" ((QWORD)(arg2)), \
        "d" ((QWORD)(arg3)) \
      : __syscall_clobber ); \
  __syscall_return (type, __res); \
}

#define _syscall4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{\
  QWORD __res; \
  __asm__ volatile ("movq %5, %%r10 ; " __syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)), "S" ((QWORD)(arg2)), \
        "d" ((QWORD)(arg3)), "g" ((QWORD)(arg4)) \
      : __syscall_clobber, "r10" ); \
  __syscall_return (type, __res); \
}

#define _syscall5(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, \
      type5, arg5) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) \
{\
  QWORD __res; \
  __asm__ volatile ("movq %5, %%r10 ; movq %6, %%r8 ; " __syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)), "S" ((QWORD)(arg2)), \
        "d" ((QWORD)(arg3)), "g" ((QWORD)(arg4)), "g" ((QWORD)(arg5)) \
      : __syscall_clobber, "r8", "r10" ); \
  __syscall_return (type, __res); \
}

#define _syscall6(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, \
      type5, arg5, type6, arg6) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) \
{\
  QWORD __res; \
  __asm__ volatile ("movq %5, %%r10 ; movq %6, %%r8 ; movq %7, %%r9 ; " __syscall \
      : "=a" (__res) \
      : "0" (SYSCALL_##name), "D" ((QWORD)(arg1)), "S" ((QWORD)(arg2)), \
        "d" ((QWORD)(arg3)), "g" ((QWORD)(arg4)), "g" ((QWORD)(arg5)), \
        "g" ((QWORD)(arg6)) \
      : __syscall_clobber, "r8", "r9", "r10" ); \
  __syscall_return (type, __res); \
}

// Functions
 
int create_thread(QWORD ip, QWORD argv, int core_mask);
QWORD get_vcon_addr(void);

int delay(QWORD sec);
int mdelay(QWORD msec);

tid_t sys_getpid(void);
int sys_fork(void);
int sys_wait(int* status);
int sys_execve(const char* name, char * const * argv, char * const * env);
int sys_getprio(tid_t* id);
int sys_setprio(tid_t* id, int prio);
void sys_exit(int arg);
ssize_t sys_read(int fd, char* buf, size_t len);
ssize_t sys_write(int fd, const char* buf, size_t len);
ssize_t sys_sbrk(ssize_t incr);
int sys_open(const char* name, int flags, int mode);
int sys_creat(const char *pathname, int mode);
int sys_close(int fd);
void sys_msleep(unsigned int ms);
int sys_sem_init(sem_t** sem, unsigned int value);
int sys_sem_destroy(sem_t* sem);
int sys_sem_wait(sem_t* sem);
int sys_sem_post(sem_t* sem);
int sys_sem_timedwait(sem_t *sem, unsigned int ms);
int sys_sem_cancelablewait(sem_t* sem, unsigned int ms);
int sys_clone(tid_t* id, void* ep, void* argv);
off_t sys_lseek(int fd, off_t offset, int whence);
size_t sys_get_ticks(void);
int sys_rcce_init(int session_id);
size_t sys_rcce_malloc(int session_id, int ue);
int sys_rcce_fini(int session_id);
void sys_yield(void);
int sys_kill(tid_t dest, int signum);
int sys_signal(signal_handler_t handler);

void do_exit(int arg);
int block_current_task(void);
void reschedule(void);
int wakeup_task(tid_t id);
int numtest(double x);
void* sys_alloc(size_t size);
BOOL sys_free(void* address);

size_t get_start_tsc(void);
size_t get_freq(void);
int sys_gettimeofday(struct timeval *tv, void *tz);
int sys_unlink(const char *path);
int sys_stat(const char *pathname, struct stat *buf);
int sys_brk(void *addr);
int sys_chdir(const char *path);

// Network offloading related systemcalls
int sys_gethostname(char *name, size_t len);
struct hostent *sys_gethostbyname(char *name);
int sys_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_listen(int sockfd, int backlog);
int sys_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen);
int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int print_log(char *msg);
char *sys3_getcwd(char *buf, size_t size);
int sys3_system(char *command); 

#endif  /* __SYSCALL_H__ */
