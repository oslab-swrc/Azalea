#include "debug.h"
#include "console.h"
#include "utility.h"
#include "console_function.h"

extern BOOL g_console_proxy_flag;    // Console proxy availability flag

/**
 * @brief Print debug information on the vcon 
 * @brief and console proxy if ready
 * @param func-name of function
 * @return none
 */
void debug_info(char *func, QWORD v, QWORD v2)
{
  static int line = 0;
  char msg[256] = {0, };

  lk_sprintf(msg, "INFO: %s %Q, %Q     \n", func, v, v2);

  lk_print_xy(35, line, msg);
  line = (line + 1) % 20;

  if (g_console_proxy_flag)
    cs_printf(msg);
}

/**
 * @brief Print debug information in specific location (x, y) on vcon
 * @brief and console proxy if ready
 * @param x-x location, y-y location, func-name of function
 * @return none
 */
void debug_info_xy(int x, int y, char *func, QWORD v, QWORD v2)
{
  char msg[256] = {0, };

  lk_sprintf(msg, "INFO: %s %Q, %Q     \n", func, v, v2);

  lk_print_xy(x, y, msg);

  if (g_console_proxy_flag)
    cs_printf(msg);
}

/**
 * @brief System halting (ERROR) with printing error messages on vcon 
 * @brief and console proxy if ready
 * @param func-name of function, v-line of code
 * @return none
 */
void debug_halt(char *func, QWORD v)
{
  char msg[256] = {0, };

  lk_sprintf(msg, "debug_halt: %s, v=%Q     \n", func, v);

  lk_print_xy(35, 23, msg);

  if (g_console_proxy_flag)
    cs_printf(msg);

  while (1)
    ;
}

/**
 * @brief System halting (NIY) with printing error messages on vcon 
 * @brief and console proxy if ready
 * @param func-name of function, v-line of code
 * @return none
 */
void debug_niy(char *func, QWORD v)
{
  char msg[256] = {0, };

  lk_sprintf(msg, "debug_niy: %s, v=%Q     \n", func, v);

  lk_print_xy(35, 23, msg);

  if (g_console_proxy_flag)
    cs_printf(msg);

  while (1)
    ;
}
