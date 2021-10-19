// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __SYSTEMCALLLIST_H__
#define __SYSTEMCALLLIST_H__

#define SYSCALL_create_thread      853

#define SYSCALL_get_vcon_addr      200
#define SYSCALL_get_cpu_num        201

#define SYSCALL_delay              300
#define SYSCALL_mdelay             301

#define SYSCALL_sys_getpid              500
#define SYSCALL_sys_fork                501
#define SYSCALL_sys_wait                502
#define SYSCALL_sys_execve              503
#define SYSCALL_sys_getprio             504
#define SYSCALL_sys_setprio             505
#define SYSCALL_sys_exit                506
#define SYSCALL_sys_read                507
#define SYSCALL_sys_write               508
#define SYSCALL_sys_sbrk                509 
#define SYSCALL_sys_open                510
#define SYSCALL_sys_close               511
#define SYSCALL_sys_msleep              512
#define SYSCALL_sys_sem_init            513
#define SYSCALL_sys_sem_destroy         514
#define SYSCALL_sys_sem_wait            515
#define SYSCALL_sys_sem_post            516
#define SYSCALL_sys_sem_timedwait       517
#define SYSCALL_sys_sem_cancelablewait  518
#define SYSCALL_sys_clone               519
#define SYSCALL_sys_lseek               520
#define SYSCALL_sys_get_ticks           521
#define SYSCALL_sys_rcce_init           522
#define SYSCALL_sys_rcce_malloc         523
#define SYSCALL_sys_rcce_fini           524
#define SYSCALL_sys_yield               525
#define SYSCALL_sys_kill                526
#define SYSCALL_sys_signal              527
#define SYSCALL_sys_creat               528
#define SYSCALL_sys_gettimeofday        529
#define SYSCALL_sys_unlink              530
#define SYSCALL_sys_stat                531
#define SYSCALL_sys_brk                 532
#define SYSCALL_sys_chdir               533
#define SYSCALL_sys_link                534
#define SYSCALL_sys_shmget              535
#define SYSCALL_sys_shmat               536
#define SYSCALL_sys_shmdt               537
#define SYSCALL_sys_shmctl              538


#define SYSCALL_do_exit                 550
#define SYSCALL_block_current_task      551
#define SYSCALL_reschedule              552
#define SYSCALL_wakeup_task             553
#define SYSCALL_numtest                 554
#define SYSCALL_sys_alloc               555
#define SYSCALL_sys_free                556

// Network offloading related systemcalls
#define SYSCALL_sys_gethostname         601
#define SYSCALL_sys_gethostbyname       602
#define SYSCALL_sys_getsockname         603
#define SYSCALL_sys_socket              604
#define SYSCALL_sys_bind                605
#define SYSCALL_sys_listen              606
#define SYSCALL_sys_connect             607
#define SYSCALL_sys_accept              608

#define SYSCALL_get_start_tsc           901
#define SYSCALL_get_freq                902
#define SYSCALL_sys3_getcwd             903
#define SYSCALL_sys3_system             904
#define SYSCALL_sys3_opendir            905
#define SYSCALL_sys3_closedir           906
#define SYSCALL_sys3_readdir            907
#define SYSCALL_sys3_rewinddir          908

#define SYSCALL_print_log               854
#endif // __SYSTEMCALLLIST_H__
