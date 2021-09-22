#!/usr/bin/zsh

protocol=('abc_tcp')#'cubiccodel' 'cubic')
trace=('Verizon-LTE-short.down')# 'Verizon-LTE-driving.up' 'TMobile-LTE-driving.up' 'ATT-LTE-driving-2016.up' 'Verizon-LTE-short.down' 'Verizon-LTE-driving.down' 'TMobile-LTE-driving.down' 'ATT-LTE-driving-2016.down')

for i in ${protocol[@]}; do
	for j in ${trace[@]}; do
		./exp.sh ${i} ${j}
	done
done
