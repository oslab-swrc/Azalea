#!/bin/bash
CORE_END=$1
if [ "$CORE_END" == "" ]
then
	CORE_END=220
fi

echo -n "${CORE_END} core(s) will be turned off"
echo ""

for ((I = 1; I <= ${CORE_END}; I++))
do
	echo 0 > /sys/devices/system/cpu/cpu${I}/online
done
