#!/bin/bash

#send to the server
#5000 15000 20000 25000 30000 40000 70000
for i in 5000 15800 22000 27800 33000 48000 93000
do
   sudo taskset -c 1,2,3,4 ./udpsender 10.0.200.2:4321 -lamda $i -psize 1450 -t 3600
   sleep 2m
done
