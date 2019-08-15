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
	BEGIN=()
elif [ $index -eq 1 ]
then
	BEGIN=()
elif [ $index -eq 2 ]
then
	BEGIN=()
elif [ $index -eq 3 ]
then
	BEGIN=()
elif [ $index -eq 4 ]
then
	BEGIN=()
elif [ $index -eq 5 ]
then
	BEGIN=()
elif [ $index -eq 6 ]
then
	BEGIN=()
else
	echo "Wrong index"
	exit 0
fi

insmod ../sideloader/lk/lk.ko
$UTIL_DIR/lkload ../disk.img $index $cpu 0 $memory 0

for ((I = 0; I < $cpu; I++))
do 
	CORE=${BEGIN[($I)]}
	$UTIL_DIR/wake $CORE
	if [ ${CORE} -eq ${BEGIN[0]} ]
	then 
		sleep 1 
	fi
done

$UTIL_DIR/print $index
