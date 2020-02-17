#!/bin/bash 

#usage: ./offload_local_proxy -o <no elements> -i <no elements> -c <no channels> -n <no nodes>
#note: no channels > core number

ulimit -n 1000000
insmod ../lk/lk.ko

./offload_local_proxy -o 64 -i 64 -c 24 -n 6
