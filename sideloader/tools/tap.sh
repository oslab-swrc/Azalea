#!/bin/sh
USER=$1
PATH=$PATH:/usr/sbin:/sbin
sudo ifconfig tap0 down
sudo tunctl -d tap0
sudo tunctl -b -u $USER 
sudo ifconfig tap0 up
sudo brctl addif virbr0 tap0
