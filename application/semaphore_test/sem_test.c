#include "utility.h"
#include "syscall.h"
#include "common.h"


sem_t *bin_sem;
static int number;
static int end_cnt;

void print_status();
void sem_snd();
void sem_rev();

void Error(const char *mes);

int y_loc = 0;

int main()
{
  int state = 0;

  state = sem_init(&bin_sem, 0);  // Semaphore initialize 
  if(state != 0)
    Error("sem_init Error");

  create_thread((QWORD)print_status, 0);

  create_thread((QWORD)sem_rev, 1);
  create_thread((QWORD)sem_rev, 2);

  create_thread((QWORD)sem_snd, 3);

  while(1) 
    if (end_cnt == 3)
      break;

  sem_destroy(bin_sem);
  print_xy(0, y_loc++%24, "sem destroy");

  return 0;
}

void print_status()
{
  while(1) {
    print_xy(0, 20, "value: %d, wpos: %d, rpos: %d [%q, %q, %q, %q, %q]  ", 
      bin_sem->value, bin_sem->wpos, bin_sem->rpos, 
      bin_sem->queue[0], bin_sem->queue[1], bin_sem->queue[2], bin_sem->queue[3], bin_sem->queue[4]);
  }
}

void sem_snd()
{
  while (number < 4) {
    delay(10);
    //print_xy(0, y_loc++%24, "%s Running : %d","snd", number);
    sem_post(bin_sem);
  }

  print_xy(0, y_loc++%24, "%s Stop : %d","snd", number);

  end_cnt++;
}

void sem_rev()
{
  int i = 0;

  for (i=0; i<2; i++) {
    print_xy(0, y_loc++%24, "%s Ready : %d","rev", number);
    sem_wait(bin_sem);
    print_xy(0, y_loc++%24, "%s Running : %d","rev", number);
    number++;
  }

  print_xy(0, y_loc++%24, "%s Stop : %d","rev", number);

  end_cnt++;
}

void Error(const char *mes)
{
  print_xy(0, y_loc++%24, "%s\n",mes);
  //exit(0);
}
