#include <sys/types.h>
#include "syscall.h"

_syscall3(int, create_thread, QWORD, ip, QWORD, argv, int, core_mask);
_syscall0(QWORD, get_vcon_addr);
_syscall0(int, get_cpu_num);

_syscall1(int, delay, QWORD, sec);
_syscall1(int, mdelay, QWORD, msec);

_syscall0(tid_t, sys_getpid);
_syscall0(int, sys_fork);
_syscall1(int, sys_wait, int*, status);
_syscall3(int, sys_execve, const char*, name, char* const*, argv, char* const*, env);
_syscall1(int, sys_getprio, tid_t*, id);
_syscall2(int, sys_setprio, tid_t*, id, int, prio);
_syscall1(void, sys_exit, int, arg);
_syscall3(ssize_t, sys_read, int, fd, char*, buf, size_t, len);
_syscall3(ssize_t, sys_write, int, fd, const char*, buf, size_t, len);
_syscall1(ssize_t, sys_sbrk, ssize_t, incr);
_syscall3(int, sys_open, const char*, name, int, flags, int, mode);
_syscall2(int, sys_creat, const char *, pathname, int, mode);
_syscall1(int, sys_close, int, fd);
_syscall1(void, sys_msleep, unsigned int, ms);
_syscall2(int, sys_sem_init, sem_t**, sem, unsigned int, value);
_syscall1(int, sys_sem_destroy, sem_t*, sem);
_syscall1(int, sys_sem_wait, sem_t*, sem);
_syscall1(int, sys_sem_post, sem_t*, sem);
_syscall2(int, sys_sem_timedwait, sem_t*, sem, unsigned int, ms);
_syscall2(int, sys_sem_cancelablewait, sem_t*, sem, unsigned int, ms);
_syscall3(int, sys_clone, tid_t*, id, void*, ep, void*, argv);
_syscall3(off_t, sys_lseek, int, fd, off_t, offset, int, whence);
_syscall0(size_t, sys_get_ticks);
_syscall1(int, sys_rcce_init, int, session_id);
_syscall2(size_t, sys_rcce_malloc, int, session_id, int, ue);
_syscall1(int, sys_rcce_fini, int, session_id);
_syscall0(void, sys_yield);
_syscall2(int, sys_kill, tid_t, dest, int, signum);
_syscall1(int, sys_signal, signal_handler_t, handler);
_syscall2(int, sys_gettimeofday, struct timeval*, tv, void*, tz);
_syscall2(int, sys_link, const char *, oldpath, const char *, newpath);
_syscall1(int, sys_unlink, const char *, path);
_syscall2(int, sys_stat, const char*, pathname, struct stat*, buf);
_syscall1(int, sys_brk, void *, addr);
_syscall1(int, sys_chdir, const char*, path);

_syscall1(void, do_exit, int, arg);
_syscall0(int, block_current_task);
_syscall0(void, reschedule);
_syscall1(int, wakeup_task, tid_t, id);
_syscall1(int, numtest, double, x);
_syscall1(void*, sys_alloc, size_t, size);
_syscall1(BOOL, sys_free, void*, address);

_syscall0(size_t, get_start_tsc);
_syscall0(size_t, get_freq);

// Network offloading related systemcalls
_syscall2(int, sys_gethostname, char *, name, size_t, len);
_syscall1(struct hostent *, sys_gethostbyname, char *, name);
_syscall3(int, sys_getsockname, int, sockfd, struct sockaddr *, addr, socklen_t *, addrlen);
_syscall3(int, sys_socket, int, domain, int, type, int, protocol);
_syscall3(int, sys_bind, int, sockfd, const struct sockaddr *, addr, socklen_t, addrlen);
_syscall2(int, sys_listen, int, sockfd, int, backlog);
_syscall3(int, sys_connect, int, sockfd, struct sockaddr *, addr, socklen_t, addrlen);
_syscall3(int, sys_accept, int, sockfd, struct sockaddr *, addr, socklen_t *, addrlen);

_syscall1(int, print_log, char*, msg);
_syscall2(char *, sys3_getcwd, char *, buf, size_t, size);
_syscall1(int, sys3_system, char*, command);
_syscall1(DIR *, sys3_opendir, const char*, name);
_syscall1(int, sys3_closedir, DIR*, dirp);
_syscall1(struct dirent*, sys3_readdir, DIR*, dirp);
_syscall1(void, sys3_rewinddir, DIR*, dirp);

//EOF
