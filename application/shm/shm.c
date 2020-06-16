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

  printf("f1 start: \n");

  for(i = 0; i < 2; i++) {
    if((shm_id[i] = shmget(KEY_NUM+i, MEM_SIZE, IPC_CREAT|0666)) == -1)
    {
      printf( "공유 메모리 생성 실패\n");
      return -1;
    }
    printf( "공유 메모리 생성shmid = %d\n", shm_id[i]);

    if(( void *)-1 == (shm_addr[i] = shmat( shm_id[i], ( void *)0, 0)))
    {
      printf( "공유 메모리 첨부 실패\n");
      return -1;
    }
    printf( "공유 메모리 첨부shmid = %d, shm_addr %lx\n\n", shm_id[i], (unsigned long) shm_addr[i]);
  }
  printf("\n");

  for(i = 0; i < 2; i++) {
    if(-1 == shmdt(shm_addr[i]))
    {
      printf( "공유 메모리 분리 실패\n");
      return -1;
    }
    else
    {
      printf( "공유 메모리 분리shm addr = %lx \n", (unsigned long) shm_addr[i]);    // 공유 메모리를 화면에 출력
    }

    if(shmctl(shm_id[i], IPC_RMID, 0) == -1) {
      printf( "공유 메모리 삭제 실패\n");
    }
    else
    {
      printf( "공유 메모리 삭제shm id = %d \n\n", shm_id[i]);    // 공유 메모리를 화면에 출력
    }
  }

  return 0;
}

int f2( void)
{
  int   shm_id;
  void *shm_addr;

  printf("f2 start: \n");

  if((shm_id = shmget(KEY_NUM2, MEM_SIZE, IPC_CREAT|0666)) == -1)
  {
    printf( "공유 메모리 생성 실패\n");
    return -1;
  }


  if ((void *)-1 == (shm_addr = shmat( shm_id, ( void *)0, 0)))
  {
    printf( "공유 메모리 첨부 실패\n");
    return -1;
  }

  while( 1 )
  {
    sprintf((char *)shm_addr, "%d", 99);       // 공유 메모리에 카운터 출력
    sleep(1);
  }

  return 0;
}

int f3( void)
{
  int   shm_id;
  void *shm_addr;

  printf("f3 start: \n");

  if((shm_id = shmget(KEY_NUM2, MEM_SIZE, IPC_CREAT|0666)) == -1)
  {
    printf( "공유 메모리 생성 실패\n");
    return -1;
  }

  if(( void *)-1 == (shm_addr = shmat(shm_id, (void *)0, 0)))
  {
     printf( "공유 메모리 첨부 실패\n");
     return -1;
  }

  while( 1 )
  {
     printf( "\rshm addr: %lx value : %s", (unsigned long) shm_addr, (char *)shm_addr);    // 공유 메모리를 화면에 출력
     sleep( 1);
  }

  return 0;
}



int main()
{
  printf("f1 create: \n");
  create_thread((QWORD)f1, 0, -1);

  printf("f2 create: \n");
  create_thread((QWORD)f2, 0, -1);

  printf("f3 create: \n");
  create_thread((QWORD)f3, 0, -1);

  return 0;
}
