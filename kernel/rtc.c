#include "rtc.h"
#include "assemblyutility.h"

/**
 * Read the current time stored in the CMOS memory by the RTC controller
 */
void read_RTC_time(BYTE *hour, BYTE *minute, BYTE *second)
{
  BYTE data;
    
  // Designating a register to store time in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
  // Read time from CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *hour = RTC_BCDTOBINARY(data);
    
  // Assign register to store minute in CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
  // Read minute from CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *minute = RTC_BCDTOBINARY(data);
    
  // Assign register to store seconds in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
  // Reads seconds from the CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *second = RTC_BCDTOBINARY(data);
}

/**
 * Read the current date stored by the RTC controller in CMOS memory
 */
void read_RTC_date(WORD *year, BYTE *month, BYTE *dayofmonth, BYTE *dayofweek)
{
  BYTE data;
    
  // Assign register to store year in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
  // Read year in the CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *year = RTC_BCDTOBINARY(data) + 2000;
    
  // Assign register to store month in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
  // Read month in the CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *month = RTC_BCDTOBINARY(data);
    
  // Assign register to store day in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
  // Read date in the CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *dayofmonth = RTC_BCDTOBINARY(data);
    
  // Assign register to store day of week in the CMOS memory address register (port 0x70)
  out_port_byte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
  // Read date of week in the CMOS data register (port 0x71)
  data = in_port_byte(RTC_CMOSDATA);
  *dayofweek = RTC_BCDTOBINARY(data);
}

/**
 * Returns the string of the day of the week using the value of the day of the week
 */
char* convert_dayofweek_tostring(BYTE dayofweek)
{
  static char* week_string[8] = { "Error", "Sunday", "Monday", 
      "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    
  if (dayofweek >= 8)
    return week_string[0];
    
  return week_string[dayofweek];
}
