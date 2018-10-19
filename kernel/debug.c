#include "debug.h"
#include "console.h"

void debug_info(char *func, QWORD v, QWORD v2)
{
  static int line = 0;

  lk_print_xy(35, line, "INFO: %s %Q, %Q   ", func, v, v2);
  line = (line + 1) % 20;
}

void debug_info_xy(int x, int y, char *func, QWORD v, QWORD v2)
{
  lk_print_xy(x, y, "INFO: %s %Q, %Q    ", func, v, v2);
}

void debug_halt(char *func, QWORD v)
{
  lk_print_xy(35, 23, "debug_halt: %s, v=%Q ", func, v);

  while (1)
    ;
}

void debug_niy(char *func, QWORD v)
{
  lk_print_xy(35, 23, "debug_niy: %s, v=%Q ", func, v);

  while (1)
    ;
}
