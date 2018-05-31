#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "ERROR: Illegal number of parameters" 
    echo "USAGE: ./process_qta_test.sh <input_file>"
    exit 1
fi

ulevel=1
q_length=0
new_time=0
time_elapsed=0
time_reference=0

while IFS='' read -r line || [[ -n "$line" ]]; do
    if [[ $line == C/U* ]] || [[ $line == Queue* ]] || [[ $line == --* ]]; then 
        continue
    fi

    if [[ $line == Urgency* ]]; then
        ulevel="${line//[!0-9]/}"
        echo "length; time" >> urgency_queue${ulevel}.csv
        old_time=0
        continue
    fi

    stringarray=($line)
    q_length=${stringarray[0]}
    new_time=${stringarray[1]}
    if [[ "$q_length" = "" ]]; then
        continue
    fi
    if [[ "$time_reference" = "0" ]]; then
        let "time_reference = $new_time"
    else
        let "time_elapsed = $new_time - $time_reference"
    fi
    echo "${q_length}; ${time_elapsed}" >> urgency_queue${ulevel}.csv
done < "$1"
