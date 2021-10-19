// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __OFFLOAD_MESSAGE_H__
#define __OFFLOAD_MESSAGE_H__

#include "offload_channel.h"

#define MAX_IOV_NUM	(8)

// structure for packet which can contain header and data.
typedef struct io_packet_struct {
  unsigned long magic;
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

void send_offload_message(struct circular_queue *ocq, int tid, int offload_function_type, QWORD param1, QWORD param2, QWORD param3, QWORD param4, QWORD param5, QWORD param6);
QWORD receive_offload_message(struct circular_queue *icq, int tid, int offload_function_type);

#endif
