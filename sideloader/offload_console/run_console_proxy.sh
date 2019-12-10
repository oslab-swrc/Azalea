#!/bin/bash 

#usage: ./console_proxy -i [index]

index=0

help()
{
        echo "Usage: ./run_console_proxy.sh -i [index]"
}

while getopts i: opt
do
        case $opt in
                i)
                        index=$OPTARG
                        ;;
                *)
                        help
                        exit 0
                        ;;
        esac
done

ulimit -n 1000000
insmod ../offload_driver/offload.ko > /dev/null 2>&1
./console_proxy -i $index

