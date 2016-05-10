#!/bin/bash
if [ "$#" -ne 3 ]; then
    echo "ERROR: Illegal number of parameters" 
    echo "USAGE: ./rmt-port-monitor.sh <ipcp_id> <n_1_port_id> <output_file>"
    exit 1
fi

echo "Monitoring IPCP $1, N-1 port $2 as fast as possible, saving data to $3"
echo "Press [CTRL+C] to stop.."

echo "//time(ms) rx_pdus tx_pdus queued_pdus dropped_pdus write_busy state" > $3

counter=1
time_ref=$(($(date +%s%N)/1000000))
while :
do
        time=$(($(($(date +%s%N)/1000000)) - time_ref))
        rx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/rx_pdus")
	tx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/tx_pdus")
	q_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/queued_pdus")
	drop_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/drop_pdus")
	wbusy=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/wbusy")
	state=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/state")

	echo "$time  $rx_pdus  $tx_pdus  $q_pdus  $drop_pdus $wbusy $state" >> $3
        counter=$((counter+1))
done
