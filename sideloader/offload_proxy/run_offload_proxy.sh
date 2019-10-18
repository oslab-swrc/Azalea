#!/bin/bash 

#usage: ./offload_local_proxy -o <no elements> -i <no elements> -c <no channels> -t <no threads>
#note: no channels > core number

ulimit -n 1000000
insmod ../offload_driver/offload.ko
#./offload_local_proxy -o 8 -i 8 -c 300 -t 10 
#./offload_local_proxy -o 32 -i 32 -c 16 -t 16
./offload_local_proxy -o 8 -i 8 -c 18 -t 18 -n 3
#./offload_local_proxy -o 4 -i 4 -c 16 -t 16 -n 1

