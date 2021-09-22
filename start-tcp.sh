#!/usr/bin/zsh

if [ $1 = "abc" ] ; then
	sudo sysctl -w net.ipv4.tcp_ecn=1
fi
if [ $1 = "abccubic" ] ; then
	sudo sysctl -w net.ipv4.tcp_ecn=1
fi
iperf -c $MAHIMAHI_BASE -p 42425 -w 16m -t 140 -Z $1 &
#iperf3 -c 192.168.1.150 -w 16m -t 140 -C $1 &
sleep 140
pkill -9 iperf
