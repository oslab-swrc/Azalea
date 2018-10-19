#!/bin/bash
CORE_END=$1
if [ "$CORE_END" == "" ]
then
	CORE_END=200
fi

echo -n "${CORE_END} core(s) will boot with lkernel"
echo ""

insmod lk.ko
#insmod 9poff.ko
insmod cmds.ko
insmod fs.ko
./lkload disk.img

sleep 1
#for ((I = 0; I < ${CORE_END}; I++))
CORE=0
#for ((I = 0; I < 54; I++))
BEGIN=40
#BEGIN=6
#((BEGIN_CORE=${BEGIN}*4))
#for ((I = ${BEGIN}; I < 59; I++))
#for ((I = ${BEGIN}; I < 60; I++))
for ((I = ${BEGIN}; I < 240; I++))
do
#    ((CORE=${I}*4+7))
#((CORE=${I}*4))
    ((CORE=${I}))
	echo $CORE
	./wake $CORE
    if [ ${CORE} -eq ${BEGIN} ]
    then
        sleep 1
    fi
done
./print_one
