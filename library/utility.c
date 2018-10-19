#include <stdarg.h>

#include "atomic.h"
#include "syscall.h"
#include "utility.h"

static QWORD vcon_addr;

/**
 * Copy memory
 */ 
int a_memcpy(void *destination, const void *source, int size)
{
  int i = 0;
  int remain_byte = 0;

  for (i=0; i<(size / 8); i++)
    ((QWORD *) destination)[i] = ((QWORD *) source)[i];

  remain_byte = i * 8;
  for (i=0; i<(size % 8); i++) {
    ((char *) destination)[remain_byte] = ((char *) source)[remain_byte];
    remain_byte++;
  }
  return size;
}

/**
 * Return the length of string
 */
int a_strlen(const char *buffer)
{
  int i = 0;

  for (i=0;; i++)
    if (buffer[i] == '\0')
      break;

  return i;
}

/**
 * Convert Integer to String
 */
int _itoa(long value, char *buffer, int radix)
{
  int ret = 0;

  switch (radix) {
  case 16:
    ret = hextostring(value, buffer);
    break;
  case 10:
  default:
    ret = decimaltostring(value, buffer);
    break;
  }

  return ret;
}

/*
 * Convert integer to string
 */
void int_to_str(int n, char *str) {
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
 * Convert HEX to String
 */
int hextostring(QWORD value, char *buffer)
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

/**
 * Convert Decimal to String
 */
int decimaltostring(long value, char *buffer)
{
  long i = 0;

  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return 1;
  }

  if (value < 0) {
    i = 1;
    buffer[0] = '-';
    value = -value;
  } else {
    i = 0;
  }

  for (; value > 0; i++) {
    buffer[i] = '0' + value % 10;
    value = value / 10;
  }
  buffer[i] = '\0';

  if (buffer[0] == '-')
    reversestring(&(buffer[1]));
  else
    reversestring(buffer);

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

  length = a_strlen(buffer);
  for (i=0; i<length / 2; i++) {
    temp = buffer[i];
    buffer[i] = buffer[length - 1 - i];
    buffer[length - 1 - i] = temp;
  }
}

/**
 *  vsprintf()
 */
int vsprintf(char *buffer, const char *parameter, va_list ap)
{
//  QWORD i = 0, k = 0;
  int i;
  int buffer_index = 0;
  int format_length = 0, copy_length = 0;
  char *copy_string = NULL;
  QWORD q_value = 0;
  int i_value = 0;
//  double d_value = 0;

  format_length = a_strlen(parameter);
  for (i=0; i<format_length; i++) {

    if (parameter[i] == '%') {
      i++;
      switch (parameter[i]) {
      // String  
      case 's':
        copy_string = (char *) (va_arg(ap, char *));
        copy_length = a_strlen(copy_string);

        a_memcpy(buffer + buffer_index, copy_string, copy_length);
        buffer_index += copy_length;
        break;
      // Char
      case 'c':
        buffer[buffer_index] = (char) (va_arg(ap, int));
        buffer_index++;
        break;

      // Integer
      case 'd':
      case 'i':
        i_value = (int) (va_arg(ap, int));
        buffer_index += _itoa(i_value, buffer + buffer_index, 10);
        break;

      // Hexadecimal (4Byte)
      case 'x':
      case 'X':
        q_value = (DWORD) (va_arg(ap, DWORD)) & 0xFFFFFFFF;
        buffer_index += _itoa(q_value, buffer + buffer_index, 16);
        break;

      // Hexadecimal (8Byte)
      case 'q':
      case 'Q':
      case 'p':
        q_value = (QWORD) (va_arg(ap, QWORD));
        buffer_index += _itoa(q_value, buffer + buffer_index, 16);
        break;

      // Floating
      case 'f':
// (double) make SSE instruction        
/*
	d_value = (double) (va_arg(ap, double));
        d_value += 0.005;
        buffer[buffer_index] = '0' + (QWORD) (d_value * 100) % 10;
        buffer[buffer_index + 1] = '0' + (QWORD) (d_value * 10) % 10;
        buffer[buffer_index + 2] = '.';
        for (k=0; ; k++) {
          if (((QWORD) d_value == 0) && (k != 0)) {
            break;
          }
          buffer[buffer_index + 3 + k] = '0' + ((QWORD) d_value % 10);
          d_value = d_value / 10;
        }
        buffer[buffer_index + 3 + k] = '\0';
        reversestring(buffer + buffer_index);
        buffer_index += 3 + k;
*/
	break;

      default:
        buffer[buffer_index] = parameter[i];
        buffer_index++;
        break;
      }
    }
    else {
      buffer[buffer_index] = parameter[i];
      buffer_index++;
    }
  }

  buffer[buffer_index] = '\0';

  return buffer_index;
}

int print( const char *buffer, ...)
{
  va_list ap;
  int ret = 0;
  char str[64];
  va_start(ap, buffer);
  ret = vsprintf(str, buffer, ap);
  va_end(ap);
  print_log(str);

  return ret;
}

/**
 * Print to screen
 */
int print_xy(int x, int y, const char *buffer, ...)
{
  va_list ap;
  int ret = 0;
  char str[1024];
  char *vscreen;
  int i = 0;
  
  if (vcon_addr == NULL)
    vcon_addr = get_vcon_addr();

#ifdef _qemu_
#define LEVEL	(2)
#else
#define LEVEL	(1)
#endif

  va_start(ap, buffer);
  ret = vsprintf(str, buffer, ap);
  va_end(ap);

  vscreen = (char *) (vcon_addr);
//  vscreen = (char *) va(*((unsigned long *) (0x94000+0x0000000000000073)) + MEMORY_START);

  vscreen += (y * 80 * LEVEL) + x*LEVEL;
  for (i = 0; str[i] != 0; i++)
    vscreen[i*LEVEL] = str[i];

  return ret;
}

int kprintf(const char *fmp, ...)
{

  return 0;
}

#if 0
/**
 * Random
 */
#define RAND_MAX 0x7fff

static long holdrand = 1L;

void srand(unsigned int seed)
{
  holdrand = (long) seed;
}

int rand()
{
  return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & RAND_MAX);
}
#endif
