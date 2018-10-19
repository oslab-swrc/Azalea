#!/bin/bash

SYSFS=/sys/lkernel
file=$SYSFS/cpu_status

if [ ! -f "$file" ]; then
  echo "0"
else
  cat $file
fi

#file=$SYSFS/thread_status

#if [ ! -f "$file" ]; then
 # echo "0 0 0 0 0 0"               
#else      
 # cat $file
#fi

echo `./threadcount` 

file=$SYSFS/pager_memory_status
if [ ! -f "$file" ]; then
  echo "0 0"               
else      
  cat $file
fi 

file=$SYSFS/ipcs_status

if [ ! -f "$file" ]; then
  echo "0 0 0 0 0 0"               
else      
  cat $file
fi 

echo `df | tail -2 | head -n 1 | awk '{print $2 " " $3}'`
