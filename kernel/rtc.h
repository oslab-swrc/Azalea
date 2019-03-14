#ifndef __RTC_H__
#define __RTC_H__

#include "az_types.h"

// Macros
// I/O PORT
#define RTC_CMOSADDRESS         0x70
#define RTC_CMOSDATA            0x71

// CMOS memory address스
#define RTC_ADDRESS_SECOND      0x00
#define RTC_ADDRESS_MINUTE      0x02
#define RTC_ADDRESS_HOUR        0x04
#define RTC_ADDRESS_DAYOFWEEK   0x06
#define RTC_ADDRESS_DAYOFMONTH  0x07
#define RTC_ADDRESS_MONTH       0x08
#define RTC_ADDRESS_YEAR        0x09

// Macro of converting BCD format to Binary
#define RTC_BCDTOBINARY(x)    ((((x) >> 4) * 10) + ((x) & 0x0F))

// Functions
void read_RTC_time(BYTE *hour, BYTE *minute, BYTE *second);
void read_RTC_date(WORD *year, BYTE *month, BYTE *dayofmonth, BYTE *dayofweek);
char* convert_dayofweek_tostring(BYTE dayofweek);

#endif /*__RTC_H__*/
