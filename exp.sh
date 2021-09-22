#!/usr/bin/zsh

if [ $1 = "abc_tcp" ] ; then
	killall iperf
	killall iperf
	sudo sysctl -w net.ipv4.tcp_ecn=1
	iperf -s -Z abc -p 42425 -w 16m &
	mm-delay 50 mm-link --uplink-log=logs/$2-$1.log --uplink-queue=cellular --uplink-queue-args=\"packets=250,qdelay_ref=50,delta=133\" mahimahi/traces/$2 mahimahi/traces/bw12.mahi ./start-tcp.sh abc
	killall iperf
	killall iperf
fi

if [ $1 = "abccubic_tcp" ] ; then
	killall iperf
	killall iperf
	sudo sysctl -w net.ipv4.tcp_ecn=1
	iperf -s -Z abccubic -p 42425 -w 16m &
	mm-delay 50 mm-link --uplink-log=logs/$2-$1.log --uplink-queue=cellular --uplink-queue-args=\"packets=250,qdelay_ref=50,delta=133\" mahimahi/traces/$2 mahimahi/traces/bw12.mahi ./start-tcp.sh abccubic
	killall iperf
	killall iperf
fi

if [ $1 = "cubic" ] ; then
	killall iperf
	killall iperf
	iperf -s -p 42425 -w 16m &
	mm-delay 50 mm-link --uplink-log=logs/$2-$1.log --uplink-queue=droptail --uplink-queue-args=\"packets=250\" mahimahi/traces/$2 mahimahi/traces/bw12.mahi ./start-tcp.sh cubic
	killall iperf
	killall iperf
fi

if [ $1 = "cubiccodel" ] ; then
	killall iperf
	killall iperf
	iperf -s -p 42425 -w 16m &
	mm-delay 50 mm-link --uplink-log=logs/$2-$1.log --uplink-queue=codel --uplink-queue-args=\"packets=250,target=50,interval=100\" mahimahi/traces/$2 mahimahi/traces/bw12.mahi ./start-tcp.sh cubic
	killall iperf
	killall iperf
fi

