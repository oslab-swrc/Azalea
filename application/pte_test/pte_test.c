#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "utility.h"
#include "syscall.h"


#define MAX_THREADS 1024*2 
//#define MAX_THREADS 128 
//#define MAX_THREADS 320 

int yloc=0;

void thread_func(void* arg)
{


#if 0
int id = sys_getpid() - 129;
int loop, i, x, y;

        x = (id % 20) * 4;
        y = (id / 20) % 24;

        for(loop = 0; loop < 32767; loop++)
          for( i = 0; i < 1000; i++);
            print_xy(x, y, "%d", i);
          print_xy(x, y, "%d ", id);
#else
int num = *((int*) arg);

        print_xy(0, yloc++%14, "Hello thread[%d] : id %d ", num, sys_getpid());
#endif
}

pthread_t threads[MAX_THREADS];
int param[MAX_THREADS];

int main(int argc, char** argv)
{
	int i, ret;
	//pthread_attr_t attr;

	pthread_init();

	//pthread_attr_init(&attr);
  	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  	
	for(i=0; i<MAX_THREADS; i++) {
		param[i] = i;
		ret = pthread_create(&threads[i], NULL, (void *)thread_func, &param[i]);
		if (ret) {
			print_xy(0, yloc++%14, "Thread creation failed! error =  %d", ret);
			return ret;
		} else {
			;//print_xy(40, yloc++%14, "Create thread %d        ", i);
		}
	}
        print_xy(0, 15, "Create thread done %d [%d]", i, MAX_THREADS*sizeof(pthread_t));

#if 1
	/* wait until all threads have terminated */
        print_xy(40, 15, "Join thread start");
	for(i=0; i<MAX_THREADS; i++) {
		pthread_join(threads[i], NULL);	
		if (ret) {
			print_xy(40, yloc++%14, "Join failed! error =  %d", ret);
			return ret;
		} else {
			print_xy(40, yloc++%14, "Join thread[%d]", i);
		}
	}
#endif


	print_xy(40, 15, "Join thread done %d       ", i);

	return 0;
}



