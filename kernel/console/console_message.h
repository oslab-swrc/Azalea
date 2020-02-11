#ifndef __CONSOLE_MESSAGE_H__
#define __CONSOLe_MESSAGE_H__ 

#include "offload_channel.h"

#define MAX_IO_PATH        (4096)
#define MAX_IOV_NUM        (32)

#define MAGIC              (0x4D3C2B1A)


// structure for packet which can contain header and data.
typedef struct io_packet_struct {
  unsigned int magic;
  unsigned short version_major;
  unsigned short version_minor;
  unsigned long packet_number;
  unsigned long total_packet_number;
  unsigned long EOP;		// End Of Packet
  unsigned long tid;
  unsigned long io_function_type;
  unsigned long param1;
  unsigned long param2;
  unsigned long param3;
  unsigned long param4;
  unsigned long param5;
  unsigned long param6;
  unsigned long ret;
} io_packet_t;

void send_console_message(struct circular_queue *ocq, int tid, int console_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6);
QWORD receive_console_message(struct circular_queue *icq, int tid, int console_function_type);

#endif //__CONSOLE_MESSAGE_H__
