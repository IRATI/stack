#!/bin/bash

oldrows=$(tput lines)             # get the number of rows
oldcolumns=$(tput cols)           # get the number of columns
old_tbf_tx_bytes=(0 0 0 0 0 0 0 0 0)
old_cu_tx_bytes=(0 0 0)

control_c()               # Function if CTRL-C is used to exit to set   
{                         #  stty sane again and remove the screen.
tput rmcup                # Removes screen
stty sane                 # Allows you to type
exit                      #Exits script
}

Draw()                           # Draw function, clears screen and echos 
{                                # whatever you want
        tput clear
        echo "Monitoring IPCP $1, N-1 port $2 every 1s"
		echo "Press [CTRL+C] to stop.."
        echo ""
        echo "    TOKEN BUCKET FILTERS (TBFs)           "
        echo ""
        for i in $( ls /sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters); do
            abs_ths=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/abs_cherish_th")
            u_level=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/urgency_level")
            prob_ths=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/prob_cherish_th")
            drop_p=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/drop_prob")
            rate=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/max_rate")
            burst=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/bucket_capacity")
            let "rate = $rate/1024"
            tx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/tbf_tx_pdus")
            tx_bytes=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/tbf_tx_bytes")
            drop_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/tbf_drop_pdus")
            drop_bytes=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/token_bucket_filters/$i/tbf_drop_bytes")
            tx_rate=0
            let "tx_rate = ($tx_bytes - ${old_tbf_tx_bytes[$i]})/128"
            old_tbf_tx_bytes[$i]=$tx_bytes
            echo "    TBF $i#  urgency_level:$u_level  abs_cher_ths:$abs_ths  prob_cher_ths:$prob_ths  drop_prob:$drop_p  max_rate(kbps):$rate  max_burst(bytes):$burst"
            echo "               tx_pdus:$tx_pdus    tx_bytes=$tx_bytes  tx_rate(kbps)=$tx_rate  drop_pdus=$drop_pdus  drop_bytes=$drop_bytes"
            echo ""
        done
        echo ""
        echo "    URGENCY QUEUES (UQs)           "
        for i in $( ls /sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/cumux/urgency_queues/); do
        	ur_level=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/cumux/urgency_queues/$i/urgency")
        	length=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/cumux/urgency_queues/$i/queued_pdus")
        	tx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/cumux/urgency_queues/$i/tx_pdus")
        	drop_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/qta_mux/cumux/urgency_queues/$i/dropped_pdus")
        	echo "    UQ $i#  urgency_level:$ur_level    length:$length"
        	echo "            tx_pdus:$tx_pdus  drop_pdus:$drop_pdus"
        	echo ""
        done
        echo ""
        tx_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/tx_pdus")
        tx_bytes=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/tx_bytes")
        drop_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/drop_pdus")
        queued_pdus=$(head -n 1 "/sys/rina/ipcps/$1/rmt/n1_ports/$2/queued_pdus")
        echo "    TOTAL(port $2): queued_pdus:$queued_pdus  drop_pdus:$drop_pdus  tx_pdus:$tx_pdus  tx_bytes:$tx_bytes"
        echo ""
        echo ""
}

ChkSize()                       #Checks any changes to the size of screen 
{                               # so you can resize Screen
        rows=$(tput lines)
        columns=$(tput cols)
        if [[ $oldcolumns != $columns || $oldrows != $rows ]];then
                Draw $1 $2
                oldcolumns=$columns
                oldrows=$rows
        fi

		Draw $1 $2
}

main()
{
    if [ "$#" -ne 2 ]; then
        echo "ERROR: Illegal number of parameters" 
        echo "USAGE: ./rmt-port-monitor.sh <ipcp_id> <n_1_port_id>"
        exit 1
    fi
	
	stty -icanon time 0 min 0       # Sets read time to nothing and prevents output on screen
	tput smcup                      # Saves current position of of cursor, and opens new screen    
	Draw $1 $2                      # Draw function

	while :
	do
    	rows=$(tput lines)      #Sets rows and cols
    	columns=$(tput cols)
    	sleep 1               #To prevent it from destroying cpu
    	ChkSize $1 $2           #Runs the checksize function
    	trap control_c SIGINT   #Traps CTRL-C for the function.
	done

	stty sane                              #Reverts stty
	tput rmcup                               #Removes screen and sets cursor back"     
	exit 0   
}

main $1 $2