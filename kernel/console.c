#include <stdarg.h>

#include "console.h"
#include "assemblyutility.h"
#include "memory_config.h"
#include "page.h"
#include "thread.h"
#include "utility.h"
#include "shellstorage.h"

unsigned long g_vcon_addr;

BYTE Shell_OP_Mode = SHELL_OP_MODE_ON;

/**
 * print formatted string 
 */
void printk(const char *fmt, ...)
{
  va_list ap;
  char buffer[1024];

  // handle variable parameter list with vsprintf()
  va_start(ap, fmt);
  lk_vsprintf(buffer, fmt, ap);
  va_end(ap);

  (void) console_prints(buffer);
}

static int line = 0;

int console_prints(const char *buff)
{
  char *p = (char *) buff;

  prints_xy(0, line, p);

  line = (line + 1) % 40;

  return line;
}

/**
 * print string at (x, y) position
 */
void prints_xy(int x, int y, char *str)
{
  int i = 0;
  char *vscreen = (char *) g_vcon_addr;

#ifdef _qemu_
#define DEPTH	 (2)
#else
#define DEPTH	 (1)
#endif

  vscreen += (y * DEPTH * CONSOLE_WIDTH) + x * DEPTH;
  for (i = 0; str[i] != 0; i++)
    vscreen[i*DEPTH] = str[i];
}

/**
 * print formatted string at (x, y) position
 */
int lk_print_xy(int x, int y, const char *parameter, ...)
{
  va_list ap;
  int return_value = 0;
  char buffer[1024];

  lk_memset(buffer, 0, sizeof(char) * 1024);

  va_start(ap, parameter);
  return_value = lk_vsprintf(buffer, parameter, ap);
  va_end(ap);

  prints_xy(x, y, buffer);

  return return_value;
}

/**
 * print formatted string to log memory
 */
int lk_print(const char *parameter, ...)
{
  va_list ap;
  int return_value = 0;
  char buffer[64];

  lk_memset(buffer, 0, sizeof(char) * 64);

  va_start(ap, parameter);
  return_value = lk_vsprintf(buffer, parameter, ap);
  va_end(ap);

  shell_enqueue(buffer);

  return return_value;
}

void int_to_str(int n, char *str)
{
  int i = 0;
  int n_temp = n;

  if(n == 0) {
    str[0] = '0';
    str[1] = 0;
  }
  else {
    while (n_temp != 0) {
      n_temp /= 10;
      i++;
    }
    str[i] = 0;

    i--;
    while (n != 0) {
      str[i] = (char) (n % 10) + '0';
      n /= 10;
      i--;
    }
  }
}

/**
 * Return the length of string
 */
int strlen(const char *buffer)
{
  int i = 0;

  for (i=0;; i++)
    if (buffer[i] == '\0')
      break;

  return i;
}


/**
 * Reverse String
 */
void reversestring(char *buffer)
{
  int length = 0;
  int i = 0;
  char temp = NULL;

  length = strlen(buffer);
  for (i=0; i<length / 2; i++) {
    temp = buffer[i];
    buffer[i] = buffer[length - 1 - i];
    buffer[length - 1 - i] = temp;
  }
}


/**
 * Convert HEX to String
*/
int hex_to_str(QWORD value, char *buffer)
{
  QWORD i = 0;
  QWORD curr_value = 0;

  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return 1;
  }

  for (i=0; value>0; i++) {
    curr_value = value % 16;

    if (curr_value >= 10)
      buffer[i] = 'A' + (curr_value - 10);
    else
      buffer[i] = '0' + curr_value;

    value = value / 16;
  }
  buffer[i] = '\0';

  reversestring(buffer);
  return i;
}



/*
 * clear screen
 */
void lk_clear_screen()
{
  int x = 0, y = 0;

  for (y = 0; y < CONSOLE_HEIGHT; y++)
    for (x = 0; x < CONSOLE_WIDTH; x++)
      prints_xy(x, y, " ");
}

/**
 * Return the address of the VCONSOLE
 */
QWORD get_vcon_addr(void)
{
  return g_vcon_addr;
}

// EOF
