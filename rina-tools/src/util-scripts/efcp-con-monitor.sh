#!/bin/bash
if [ "$#" -ne 3 ]; then
    echo "ERROR: Illegal number of parameters" 
    echo "USAGE: ./rmt-port-monitor.sh <ipcp_id> <efcp_con_src_cep_id> <output_file>"
    exit 1
fi

echo "Monitoring IPCP $1, EFCP connection $2 as fast as possible, saving data to $3"
echo "Press [CTRL+C] to stop.."

echo "//time(ms) rx_pdus tx_pdus dropped_pdus rx_credit tx_credit snd_window_size" > $3

counter=1
time_ref=$(($(date +%s%N)/1000000))
while :
do
        time=$(($(($(date +%s%N)/1000000)) - time_ref))
        drop_pdus=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtp/drop_pdus")
        tx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtp/tx_pdus")
	rx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtp/rx_pdus")
        rx_credit=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtcp/rx_credit")
        tx_credit=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtcp/tx_credit")
        snd_window_size=$(head -n 1 "/sys/rina/ipcps/$1/connections/$2/dtcp/snd_window_size")
        echo "$time $drop_pdus $tx_pdus $rx_pdus $rx_credit $tx_credit $snd_window_size" >> $3
        counter=$((counter+1))
done
