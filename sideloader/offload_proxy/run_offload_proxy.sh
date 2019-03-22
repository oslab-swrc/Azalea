#!/bin/bash 
ulimit -n 1000000
insmod ../offload_driver/offload.ko
./offload_local_proxy -o 8 -i 8 -c 300 -t 10 
