#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "utility.h"
#include "syscall.h"

static void sighandler(int sig)
{
  print_xy(0, 15, "#################");

  while(1)
    ;
}

#if 0
void malloc_test()
{
  int *tmp_ptr;
  int tid = stid++;
  QWORD cnt_s = 0;
  QWORD cnt_f = 0;

  while(1) {
    tmp_ptr = malloc(3*1024);
//    *tmp_ptr = 1000+i;
    if (tmp_ptr <= 0)
      print_xy(30, 13+tid, "## %q, %q", tmp_ptr, cnt_f++);
    else
      print_xy(0, 13+tid, "## %q, %q", tmp_ptr, cnt_s++);

//   free(tmp_ptr);
  }
}
#endif

void malloc_test1(void)
{
  QWORD *ptr;
  QWORD cnt_s = 0;
  QWORD cnt_f = 0;

  print_xy(0, 11, "1 START");

  while(1) {
    ptr = malloc(3*1024);
    *ptr = 1000+cnt_s;

    if (ptr <= 0)
      print_xy(0, 13, "F# %q, %d", ptr, cnt_f++);
    else
      print_xy(0, 12, "S# %q, %d", ptr, cnt_s++);

    free(ptr);
  }

  print_xy(0, 11, "1 END   ");
}

void malloc_test2(void)
{
  QWORD *ptr;
  QWORD cnt_s = 0;
  QWORD cnt_f = 0;

  print_xy(20, 11, "2 START");

  while(1) {
    ptr = malloc(5*1024);
    *ptr = 1000+cnt_s;

    if (ptr <= 0)
      print_xy(20, 13, "F# %q, %d", ptr, cnt_f++);
    else
      print_xy(20, 12, "S# %q, %d", ptr, cnt_s++);

    free(ptr);
  }

  print_xy(20, 11, "2 END   ");
}

#define ITER     10000

QWORD *g_ptr[ITER];

void malloc_test3(void) 
{
//  QWORD *ptr[ITER];
  QWORD cnt_s = 0;
  QWORD cnt_f = 0;
  int i;

  print_xy(40, 11, "3 START");

  // ALLOC
  for (i=0; i<ITER; i++) {
    g_ptr[i] = malloc(4*1024);
    *g_ptr[i] = 1000+cnt_s;

    if (g_ptr[i] <= 0)
      print_xy(40, 13, "F#A: %q, %d", g_ptr[i], cnt_f++);
    else
      print_xy(40, 12, "S#A: %q, %d", g_ptr[i], cnt_s++);
  }

  // FREE
  for (i=0; i<ITER; i++) {
    free(g_ptr[i]);
    print_xy(40, 14, "##F: %q, %d", g_ptr[i], i);
  }

  print_xy(40, 11, "3 END   ");
} 

void malloc_test4(void) 
{
  QWORD *ptr;
  QWORD cnt_s = 0;
  QWORD cnt_f = 0;
  int i;
  srand(10000);

  print_xy(60, 11, "4 START");

  // ALLOC
  for (i=0; i<ITER; i++) {
    int random = (rand() % 4096) + 1000;
  
    ptr = malloc(random);
    *ptr = 1000+cnt_s;

    if (ptr <= 0)
      print_xy(60, 13, "F# %q, %q, %d", ptr, random, cnt_f++);
    else
      print_xy(60, 12, "S# %q, %q, %d", ptr, random, cnt_s++);

    free(ptr);
  }

  print_xy(60, 11, "4 END   ");
} 

void signal_test(void)
{
  signal(16, sighandler);
}

void test(void *arg)
{
  int param = *(int*) arg;
  int cnt = 0;

  while(1) {
    print_xy(0, 14, "test thread: %d, %q", param, cnt++);
  }
}

void pthread_test()
{
  pthread_t t;
  int ret;
  int i = 100;

  ret = pthread_create(&t, NULL, (void *)test, &i);
  if (ret) {
    print_xy(0, 13, "thread creation failed");
  } else {
    print_xy(0, 13, "thread creation success");
  }

  while(1)
    ;
}

int main()
{
#if 0 // malloc_test
  create_thread((QWORD) malloc_test1, 0, 1);
  delay(2);

  create_thread((QWORD) malloc_test2, 0, 2);
  delay(2);

  create_thread((QWORD) malloc_test3, 0, 3);
  delay(2);

  create_thread((QWORD) malloc_test4, 0, 4);
#endif

#if 0
  create_thread((QWORD) signal_test, 1);
//  delay(10);
#endif

#if 1
  pthread_test();
#endif

  while(1)
    ;

  return 0;
}
