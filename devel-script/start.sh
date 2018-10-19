#!/bin/bash

UTIL_DIR=../sideloader/utils

cpu=0
memory=0
index=0

help()
{
	echo "Usage: ./start.sh -i [index] -c [cpu] -m [memory]"
}

while getopts c:m:i: opt
do
	case $opt in
		i)
			index=$OPTARG
			;;
		c)
			cpu=$OPTARG
			;;
		m)
			memory=$OPTARG
			;;
		*)
			help
			exit 0
			;;
	esac
done

if [ $index -eq 0 ]
then
	if [ $cpu -eq 0 ] || [ $memory -eq 0 ]
	then
		help
		exit 0
	fi
	BEGIN=$cpu
	END=$(($cpu + 5))
else
	BEGIN=$(($index * 5 + 15))
	END=$(($BEGIN + 5))
fi

insmod ../sideloader/lk/lk.ko
$UTIL_DIR/lkload ../disk.img $index $cpu 0 $memory 0

for ((I = ${BEGIN}; I < ${END}; I++))
do 
	CORE=${I}
	$UTIL_DIR/wake $CORE
	if [ ${CORE} -eq ${BEGIN} ]
	then 
		sleep 1 
	fi
done

$UTIL_DIR/print $(($index -1))
