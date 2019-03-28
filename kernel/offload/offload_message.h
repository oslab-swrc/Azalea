#ifndef __OFFLOAD_MESSAGE_H__
#define __OFFLOAD_MESSAGE_H__

#include "offload_channel.h"

#define IO_PATH_MAX        (4096)
#define MAGIC              (0x4D3C2B1A)
#define MAX_IOV_SIZE       (512*0x1000) / sizeof(struct iovec)
#define MAX_IOV_NUM        (512)

// structure for buffer which can contain pathname.
typedef struct io_path_struct {
  //unsigned long length;
  unsigned long length;
  unsigned char data[IO_PATH_MAX];
} io_path_t;

// structure for buffer which can contain data.
typedef struct io_buffer_struct {
  //unsigned long length;
  unsigned long length;
  unsigned char data[0] __attribute__((aligned(4096)));
} io_buffer_t;

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
  io_path_t path;
  io_buffer_t buffer;
} io_packet_t;

void send_offload_message(struct circular_queue *ocq, int tid, int offload_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6);
QWORD receive_offload_message(struct circular_queue *icq, int tid, int offload_function_type);

#endif
