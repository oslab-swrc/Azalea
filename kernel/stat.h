#ifndef __STAT_H__
#define __STAT_H__

typedef struct {
  int total_core;
  int used_core;
  int total_mem;
  int used_mem;
} Rmanager;

typedef struct {
  Rmanager resource_manager;
} STAT_AREA;

#endif  /* __STAT_H__ */

