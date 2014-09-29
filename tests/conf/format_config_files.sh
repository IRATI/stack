#!/bin/bash

for i in $(ls ipcmanager.conf*); do
        echo "Formatting '$i'"
        cp $i ttt
        cat ttt | json_reformat > $i
done
rm ttt
