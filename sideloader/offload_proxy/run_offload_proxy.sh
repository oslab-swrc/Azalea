#!/bin/bash 

#usage: ./offload_local_proxy -o <no elements> -i <no elements> -c <no channels> -t <no threads>
#note: no channels > core number

ulimit -n 1000000
insmod ../offload_driver/offload.ko
./offload_local_proxy -o 8 -i 8 -c 300 -t 10 
