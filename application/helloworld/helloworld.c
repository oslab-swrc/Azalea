#include "utility.h"
#include "syscall.h"

static int xxloc = 0;
static int yyloc = 3;

void hello()
{
  int i = 0;
  int xloc = xxloc;
  int yloc = yyloc; 
  yyloc++;

  print_xy(xloc, yloc, "stack_t: %q, ", &i);

  while(1) {
    print_xy(xloc + 25, yloc, "cnt: %q  ", i++);
  }
}

void hello_exit()
{
  int i = 0;
  int xloc = xxloc;
  int yloc = yyloc; 
  yyloc++;

  print_xy(xloc, yloc, "stack_t: %q, ", &i);

  print_xy(xloc + 25, yloc, "cnt: %q  ", i++);
}

int main()
{
  int i;
  print_xy(0, 0, "stack_main: %q, ", &i);

  for (i=0; i<3; i++) {
    create_thread((QWORD)hello, 0, -1);
  }

  for (i=0; i<3; i++) {
    create_thread((QWORD)hello, 0, -1);
  }

  for (i=0; i<3; i++) {
    create_thread((QWORD)hello_exit, 0, -1);
  }

  for (i=0; i<3; i++) {
    create_thread((QWORD)hello_exit, 0, -1);
  }

  while(1) {
    print_xy(25, 0, "cnt: %q  ", i++);
  }

  return 0;
}
