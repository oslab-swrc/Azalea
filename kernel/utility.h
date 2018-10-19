#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "types.h"
#include <stdarg.h>

// Macro
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))

// Functions
void lk_memset(void *address, BYTE data, int size);
int lk_memcpy(void *destination, const void *source, int size);
int lk_memcmp(const void *destination, const void *source, int size);
int lk_strlen(const char *buffer);

int lk_itoa(long value, char *buffer, int radix);
int lk_hextostring(QWORD value, char *buffer);
int lk_decimaltostring(long value, char *buffer);
void lk_reversestring(char *buffer);

int lk_sprintf(char *buffer, const char *parameter, ...);
int lk_vsprintf(char *buffer, const char *format_string, va_list ap);

void lk_delay(long d1, long d2);
void lk_srand(unsigned int seed);
int lk_rand();

void lk_memory_test();
BOOL check_memory(QWORD size);

void measure_processor_speed(void);
void show_date_and_time(void);

#endif /*__UTILITY_H__*/
