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
elif [ $index -eq 1 ]
then
        BEGIN=(8 12 40 44 72 76 104 108 136 140 168 172 200 204 232 236)
elif [ $index -eq 2 ]
then
        BEGIN=(48 52 80 84 112 116 144 148 176 180 192 196 208 212 240 244)
elif [ $index -eq 3 ]
then
        BEGIN=(24 28 56 60 88 92 120 124 152 156 184 188 216 220 248 252)
else
        echo "Wrong index"
        exit 0
fi

insmod ../sideloader/lk/lk.ko
$UTIL_DIR/lkload ../disk.img $index $cpu 0 $memory 0

for ((I = 0; I < ${#BEGIN[*]}; I++))
do 
	CORE=${BEGIN[($I)]}
	$UTIL_DIR/wake $CORE
	if [ ${CORE} -eq ${BEGIN[0]} ]
	then 
		sleep 1 
	fi
done

$UTIL_DIR/print $(($index -1))
