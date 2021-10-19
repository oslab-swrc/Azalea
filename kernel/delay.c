// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "delay.h"
#include "pit.h"

int az_delay(QWORD sec)
{
  int i;

  
  for (i=0; i<20*sec; i++)
    wait_using_directPIT( MSTOCOUNT(50) );

  return 0;
}

int az_mdelay(QWORD msec)
{
  wait_using_directPIT( MSTOCOUNT(msec) );

  return 0;
}
