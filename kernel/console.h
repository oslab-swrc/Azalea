#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "az_types.h"

#define CONSOLE_WIDTH			80
#define CONSOLE_HEIGHT			25

#define	SHELL_OP_MODE_OFF		0
#define	SHELL_OP_MODE_ON		1

extern BYTE Shell_OP_Mode;

// Functions
int console_prints(const char *buff);
void printk(const char *fmt, ...);
void prints_xy(int x, int y, char *str);
int lk_print_xy(int x, int y, const char *parameter, ...);
int lk_print(const char *parameter, ...);

void lk_clear_screen(void);
void on_off_status(int phy, char state);
void int_to_str(int input, char* output);
int hex_to_str(QWORD value, char *buffer);

QWORD get_vcon_addr(void);

#endif  /* __CONSOLE_H__ */
