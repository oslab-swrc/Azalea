#ifndef __STAT_H__
#define __STAT_H__

#include "cmds.h"

typedef struct {
  int used;
  char name[100];                     // name of the application
  int total_core_num;                 // total core num.
  int core[MAX_PROCESSOR_COUNT];      // core physical id
  int mem_size;                       // total memory size
  QWORD mem_start;                    // start memory address
  QWORD mem_used;                     // used memory size
  QWORD start_time;                   // the time at unikernel started
} Unikernel;

typedef struct {
  int plimit;                         // power limit (Watt).
  int core_used[MAX_CORE];            // core allocation info.
  int core_load[MAX_CORE];            // core load info.
  int memory_used[MAX_MEMORY];        // memory allocation info.
  int unikernel_cnt;                  // total num of launched unikernels
  Unikernel ukernel[MAX_UNIKERNEL];   // unikernel info. based on its id
} STAT_AREA;

#endif  /* __STAT_H__ */
