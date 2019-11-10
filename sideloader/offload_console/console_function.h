#ifndef __CONSOLE_FUNCTION_H__
#define __CONSOLE_FUNCTION_H__

#include "console_channel.h"
#include "console_message.h"

#define CONSOLE_PRINT   1
#define CONSOLE_WRITE   2
#define CONSOLE_GETCH   3
#define CONSOLE_EXIT	(-9)

void console_off_write(struct channel_struct *ch);
void console_print(struct channel_struct *ch);
void console_getch(struct channel_struct *ch);

#endif /*__CONSOLE_FUNCTION_H__*/
