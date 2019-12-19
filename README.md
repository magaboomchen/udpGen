# udpGen
udpGen is based on another project. (https://github.com/majek/dump/tree/master/how-to-receive-a-million-packets)

## Features
udpGen has some features:

(1) Packet-level delay measurements: By adding timestamp in IP Option fileds.

(2) Traffic control: By adding PID algorithm to keep real-time sending speed to objective speed.

(3) Different traffic pattern: Support possion traffic and constant speed traffic.

(4) Packet payload size: Support customized packet size.

(5) Running time control: udpGen will stop sending traffic after running a period of time.

## Prerequisite
udpGen dosen't need dpdk.

clang

## build
(1) cd ./udpGen

(2) chmod +x ./build.sh

(3) build.sh

## Usage
First, prepare two machines.

Machine A (ip=10.0.200.1) is sender and machine B (ip=10.0.200.2) is reciever.

### send
On machine A, run:

./udpsender 10.0.200.2:4321 -lamda 1000 -psize 1 -t 150

This means udpsender send possion traffic (lamda=1000 packet per seconds, pps) to  10.0.200.2:4321; each packet in traffic has payload size 1; udpGen will stop after 150 seconds later.

You may find that when lamda is big, udpGen can't achieve expected speed.

Then you should add more thread to speed up.

On machine A, run:

sudo taskset -c 1,2,3,4 ./udpsender 10.0.200.2:4321 -lamda 100000 -psize 1 -t 150

### recieve
On machine B, run:

udpreceiver1

This program will listen on port 4321 and display the traffic information.
