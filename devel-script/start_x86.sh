#!/bin/bash

UTIL_DIR=../sideloader/utils
CONSOLE_DIR=../sideloader/offload_console

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
#	BEGIN=(64 66 68 70 72 74 80 82 84 86 88 90 96 98 100 102 104 106 112 114 116 118 120 122)
	BEGIN=(128 130 132 134 136 138 144 146 148 150 152 154 160 162 164 166 168 170 176 178 180 182 184 186 192 194 196 198 200 202 208 210 212 214 216 218 224 226 228 230 232 234 240 242 244 246 248 250)
elif [ $index -eq 1 ]
then
#	BEGIN=(128 130 132 134 136 138 144 146 148 150 152 154 160 162 164 166 168 170 176 178 180 182 184 186)
	BEGIN=(256 258 260 262 264 266 272 274 276 278 280 282 288 290 292 294 296 298 304 306 308 310 312 314 320 322 324 326 328 330 336 338 340 342 344 346 352 354 356 358 360 362 368 370 372 374 376 378)
elif [ $index -eq 2 ]
then
#	BEGIN=(192 194 196 198 200 202 208 210 212 214 216 218 224 226 228 230 232 234 240 242 244 246 248 250)
	BEGIN=(384 386 388 390 392 394 400 402 404 406 408 410 416 418 420 422 424 426 432 434 436 438 440 442 448 450 452 454 456 458 464 466 468 470 472 474 480 482 484 486 488 490 496 498 500 502 504 506)
elif [ $index -eq 3 ]
then
	BEGIN=(256 258 260 262 264 266 272 274 276 278 280 282 288 290 292 294 296 298 304 306 308 310 312 314)
elif [ $index -eq 4 ]
then
	BEGIN=(320 322 324 326 328 330 336 338 340 342 344 346 352 354 356 358 360 362 368 370 372 374 376 378)
elif [ $index -eq 5 ]
then
	BEGIN=(384 386 388 390 392 394 400 402 404 406 408 410 416 418 420 422 424 426 432 434 436 438 440 442)
elif [ $index -eq 6 ]
then
	BEGIN=(448 450 452 454 456 458 464 466 468 470 472 474 480 482 484 486 488 490 496 498 500 502 504 506)
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

insmod ../sideloader/offload_driver/offload.ko  > /dev/null 2>&1
$CONSOLE_DIR/console_proxy -i $index
