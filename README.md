# ABC-NSDI2020
This repo contains the code for the congestion control protocol ABC. Please see our paper for more details: https://www.usenix.org/system/files/nsdi20-paper-goyal.pdf.

##Relevant Files
1. mahimahi/src/packet/cellular_packet_queue.cc: This file contains logic for the ABC router
2. tcp_abc.c: Linux endpoint for basic ABC
3. tcp_abccubic.c: Linux endpoint for ABC with modifications for coexistence with non-ABC routers (see section 4.1 of the paper)
4. run-exp.sh: shows an example on how to run an experiment

## Setup:
1. Install Mahimahi: 
a. cd mahimahi
b. ./autogen.sh
c. ./configure
d. make && sudo make install

2. Install ABC endpoints
a. make
b. sudo insmod abc.ko
c. cat /proc/sys/net/ipv4/tcp_available_congestion_control | sudo tee /proc/sys/net/ipv4/tcp_allowed_congestion_control

