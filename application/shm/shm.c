#include <stdio.h>      // printf()
#include <unistd.h>     // sleep()
#include <sys/ipc.h>
#include <sys/shm.h>

#include "syscall.h"

#define  KEY_NUM     9534
#define  KEY_NUM2    8534
#define  MEM_SIZE    1024

#define	shmget	sys_shmget
#define	shmat	sys_shmat
#define	shmdt	sys_shmdt
#define	shmctl	sys_shmctl
#define	sleep	sys_msleep

int f1( void)
{
  int   shm_id[10];
  void *shm_addr[10];
  int i = 0;

  printf("f1 start: shmget()->shmat()->shmdt()->shmctl()\n");

  for(i = 0; i < 2; i++) {
    if((shm_id[i] = shmget(KEY_NUM+i, MEM_SIZE, IPC_CREAT|0666)) == -1)
    {
      printf( "Shared memory create failed.\n");
      return -1;
    }
    printf( "Shared memory create [pass]: shmid = %d\n", shm_id[i]);

    if(( void *)-1 == (shm_addr[i] = shmat( shm_id[i], ( void *)0, 0)))
    {
      printf( "Shared memory attach failed.\n");
      return -1;
    }
    printf( "Shared memory attach [pass]: shmid = %d, shm addr %lx\n\n", shm_id[i], (unsigned long) shm_addr[i]);
  }
  printf("\n");

  for(i = 0; i < 2; i++) {
    if(-1 == shmdt(shm_addr[i]))
    {
      printf( "Shared memory detach failed.\n");
      return -1;
    }
    else
    {
      printf( "Shared memory detach [pass]: shm addr = %lx \n", (unsigned long) shm_addr[i]);    // 공유 메모리를 화면에 출력
    }

    if(shmctl(shm_id[i], IPC_RMID, 0) == -1) {
      printf( "Shared memory delete failed\n");
    }
    else
    {
      printf( "Shared memory delete [pass]: shmid = %d \n\n", shm_id[i]);    // 공유 메모리를 화면에 출력
    }
  }

  return 0;
}

int f2( void)
{
  int   shm_id = -1;
  void *shm_addr = NULL;
  int count = 0;

  printf("f2 start: write\n");

  if((shm_id = shmget(KEY_NUM2, MEM_SIZE, IPC_CREAT|0666)) == -1)
  {
    printf( "Shared memory create failed.\n");
    return -1;
  }


  if ((void *)-1 == (shm_addr = shmat( shm_id, ( void *)0, 0)))
  {
    printf( "Shared memory attach failed.\n");
    return -1;
  }

  while( 1 )
  {
    sprintf((char *)shm_addr, "%d", count++ % 1000);       // 공유 메모리에 카운터 출력
    if(count == 1000) {
      count = 0;
    }
    sleep(10);
  }

  return 0;
}

int f3( void)
{
  int   shm_id = -1;
  void *shm_addr = NULL;

  printf("f3 start: read\n");

  if((shm_id = shmget(KEY_NUM2, MEM_SIZE, IPC_CREAT|0666)) == -1)
  {
    printf( "Shared memory create failed.\n");
    return -1;
  }

  if(( void *)-1 == (shm_addr = shmat(shm_id, (void *)0, 0)))
  {
    printf( "Shared memory attach failed.\n");
    return -1;
  }

  while( 1 )
  {
    printf( "\rf3 shm addr: %lx, value: %s  ", (unsigned long) shm_addr, (char *)shm_addr);    // 공유 메모리를 화면에 출력
    sleep(10);
  }

  return 0;
}



int main()
{
  printf("f1 funcion execute: \n");
  f1();

  printf("f2 thread create: \n");
  create_thread((QWORD)f2, 0, -1);

  printf("f3 thread create: \n");
  create_thread((QWORD)f3, 0, -1);

  return 0;
}
