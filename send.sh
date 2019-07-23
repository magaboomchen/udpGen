#!/bin/sh

#sudo taskset -c 1,2,3,4,5,6,7,8 ./udpsender 10.0.200.2:4321 10.0.200.2:4321 10.0.200.2:4321 10.0.200.2:4321 -lamda $1 -psize 1 -t 5

sudo taskset -c 1,2,3,4 ./udpsender 10.0.200.2:4321 -lamda $1 -psize 1 -t 150

