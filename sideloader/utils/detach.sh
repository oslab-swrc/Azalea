#!/bin/bash
CORE_START=$1
NUM_CORES=$2
if [ "$CORE_START" == "" ]
then
	CORE_START=4
fi
if [ "$NUM_CORES" == "" ]
then
	NUM_CORES=6
fi
CORE_END=$(($CORE_START + $NUM_CORES - 1))

echo -n "${NUM_CORES} core(s) [${CORE_START} - ${CORE_END}]"
echo -n " will be turned off..."

for ((I = ${CORE_START}; I <= ${CORE_END}; I++))
do
	echo 0 > /sys/devices/system/cpu/cpu${I}/online
done
echo "done"
