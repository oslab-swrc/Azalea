#ifndef __OFFLOAD_MESSAGE_H__
#define __OFFLOAD_MESSAGE_H__

#include <stddef.h>

#include "offload_channel.h"

#define PACKET_DATA_SIZE  (128 * 4096)
#define IO_PATH_MAX	  (4096)
#define MAGIC		  (0x4D3C2B1A)

// structure for buffer which can contain pathname.
typedef struct io_path_struct {
  size_t length;
  unsigned char data[IO_PATH_MAX]; 
} io_path_t;

// structure for buffer which can contain data.
typedef struct io_buffer_struct {
  size_t length;		
  unsigned char data[0] __attribute__((aligned(4096)));
} io_buffer_t;	

// structure for packet which can contain header and data.
typedef struct io_packet_struct {
  unsigned int magic;
  unsigned short version_major;
  unsigned short version_minor;
  unsigned long packet_number;
  unsigned long total_packet_number;  
  unsigned long EOP;
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

void send_sys_message(struct circular_queue *out_cq, int tid, int offload_function_type, unsigned long  ret);

#endif
