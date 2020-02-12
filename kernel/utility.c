#include "utility.h"
#include "console.h"
#include "page.h"
#include "atomic.h"
#include "assemblyutility.h"
#include "pit.h"
#include "rtc.h"

#include <stdarg.h>

extern QWORD g_memory_start;
extern QWORD g_memory_end;

/**
 *  Fill input data into the memory
 */
// TODO:
void lk_memset(void *address, BYTE data, int size)
{
#if 0
  int i = 0;
  QWORD tmp_data = 0;
  int remain_offset = 0;
  QWORD addr = (QWORD) address;

  // Fill 8 byte data
  tmp_data = 0;
  for (i=0; i<8; i++)
    tmp_data = (tmp_data << 8) | data;

  // Fill 8 byte size data
  for (i=0; i<(size / 8); i++) {
    *((QWORD *) addr) = tmp_data;
    addr++;
  }

  // Fill the rest
  remain_offset = i * 8;
  for (i=0; i<(size % 8); i++) 
    ((char *) addr)[remain_offset++] = data;
#else
  int i = 0;
  QWORD tmp_data = 0;
  int remain_offset = 0;

  // Fill 8 byte data
  tmp_data = 0;
  for (i=0; i<8; i++)
    tmp_data = (tmp_data << 8) | data;

  // Fill 8 byte size data
  for (i=0; i<(size / 8); i++) {
    *((QWORD *) address) = tmp_data;
    address++;
  }

  // Fill the rest
  remain_offset = i * 8;
  for (i=0; i<(size % 8); i++) 
    ((char *) address)[remain_offset++] = data;
#endif
}

/**
 * Copy memory
 */ 
int lk_memcpy(void *destination, const void *source, int size)
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
 * Compare memory 
 */
int lk_memcmp(const void *destination, const void *source, int size)
{
  int i = 0, j = 0;
  int start_offset = 0;
  QWORD value = 0;
  char ch_value = NULL;

  for (i=0; i<(size / 8); i++) {
    value = ((QWORD *) destination)[i] - ((QWORD *) source)[i];

    if (value != 0)
      for (j=0; j<8; j++)
        if (((value >> (j * 8)) & 0xFF) != 0)
          return (value >> (j * 8)) & 0xFF;
  }

  start_offset = i * 8;
  for (i=0; i<(size % 8); i++) {
    ch_value = ((char *) destination)[start_offset] - ((char *) source)[start_offset];
    if (ch_value != 0) {
      return ch_value;
    }
    start_offset++;
  }
  return 0;
}

/**
 * Return the length of string
 */
int lk_strlen(const char *buffer)
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
int lk_itoa(long value, char *buffer, int radix)
{
  int ret = 0;

  switch (radix) {
  case 16:
    ret = lk_hextostring(value, buffer);
    break;
  case 10:
  default:
    ret = lk_decimaltostring(value, buffer);
    break;
  }

  return ret;
}

/**
 * Convert HEX to String
 */
int lk_hextostring(QWORD value, char *buffer)
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

  lk_reversestring(buffer);
  return i;
}

/**
 * Convert Decimal to String
 */
int lk_decimaltostring(long value, char *buffer)
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
    lk_reversestring(&(buffer[1]));
  else
    lk_reversestring(buffer);

  return i;
}

/**
 * Reverse String
 */
void lk_reversestring(char *buffer)
{
  int length = 0;
  int i = 0;
  char temp = NULL;

  length = lk_strlen(buffer);
  for (i=0; i<length / 2; i++) {
    temp = buffer[i];
    buffer[i] = buffer[length - 1 - i];
    buffer[length - 1 - i] = temp;
  }
}

/**
 *  sprintf()
 */
int lk_sprintf(char *buffer, const char *parameter, ...)
{
  va_list ap;
  int ret;

  va_start(ap, parameter);
  ret = lk_vsprintf(buffer, parameter, ap);
  va_end(ap);

  return ret;
}

/**
 *  vsprintf()
 */
int lk_vsprintf(char *buffer, const char *parameter, va_list ap)
{
  QWORD i = 0, k = 0;
  int buffer_index = 0;
  int format_length = 0, copy_length = 0;
  char *copy_string = NULL;
  QWORD q_value = 0;
  int i_value = 0;
  double d_value = 0;

  format_length = lk_strlen(parameter);
  for (i=0; i<format_length; i++) {

    if (parameter[i] == '%') {
      i++;
      switch (parameter[i]) {
      // String  
      case 's':
        copy_string = (char *) (va_arg(ap, char *));
        copy_length = lk_strlen(copy_string);

        lk_memcpy(buffer + buffer_index, copy_string, copy_length);
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
        buffer_index += lk_itoa(i_value, buffer + buffer_index, 10);
        break;

      // Hexadecimal (4Byte)
      case 'x':
      case 'X':
        q_value = (DWORD) (va_arg(ap, DWORD)) & 0xFFFFFFFF;
        buffer_index += lk_itoa(q_value, buffer + buffer_index, 16);
        break;

      // Hexadecimal (8Byte)
      case 'q':
      case 'Q':
      case 'p':
        q_value = (QWORD) (va_arg(ap, QWORD));
        buffer_index += lk_itoa(q_value, buffer + buffer_index, 16);
        break;

      // Floating
      case 'f':
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
        lk_reversestring(buffer + buffer_index);
        buffer_index += 3 + k;
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

/**
 * Delay
 */
void lk_delay(long d1, long d2)
{
  int i = 0, j = 0;

  for (i=0; i<d1; i++)
    for (j=0; j<d2; j++)
      ;
}

/**
 * Random
 */
#define RAND_MAX 0x7fff

static long holdrand = 1L;

void lk_srand(unsigned int seed)
{
  holdrand = (long) seed;
}

int lk_rand()
{
  return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & RAND_MAX);
}

/**
 * @brief Check error on free memory area
 * @return On success, TRUE is returned. On error, FALSE is returned
 */
BOOL az_check_memory()
{
  QWORD* curr;
  QWORD prev;

  //Start of check in 4Mbyte unit from free memory + 1GB
  curr = (QWORD*) va(g_memory_start + (1 << 30));

  while((QWORD) curr < va(g_memory_end)) {
    // Store the previously memory value
    prev = *curr;
    // Recognized as a valid memory area until no problem when save and read same value
    *curr = 0x12345678;
    if (*curr != 0x12345678) {
      return FALSE;
    }

    // Restore to previous memory value
    *curr = prev;

    // Move to next 4MB memory 
    curr += ( 4 << 20 );
  }

  return TRUE;
}

/**
 *  Calculate the speed of the Processor
 */
void measure_processor_speed(void)
{
  int i;
  QWORD last_TSC, total_TSC = 0;

  // Indirectly measure the speed of the processor with a 10 second time stamp counter
  disable_interrupt();
  for (i=0; i<200; i++) {
    last_TSC = read_tsc();
    wait_using_directPIT((WORD)MSTOCOUNT(50));
    total_TSC += read_tsc() - last_TSC;
  }
  enable_interrupt();

  lk_print_xy(0, 22, "CPU Speed = %d MHz", total_TSC / 10 / 1000 / 1000);
}

/**
 * Displays the date and time information stored in the RTC controller
 */
void show_date_and_time(void)
{
  BYTE second, minute, hour;
  BYTE dayofweek, dayofmonth, month;
  WORD year;

  read_RTC_time(&hour, &minute, &second);
  read_RTC_date(&year, &month, &dayofmonth, &dayofweek);

  lk_print_xy(0, 23, "Date: %d/%d/%d %s", year, month, dayofmonth, convert_dayofweek_tostring(dayofweek));
  lk_print_xy(0, 24, "Time: %d:%d:%d", hour, minute, second);
}

