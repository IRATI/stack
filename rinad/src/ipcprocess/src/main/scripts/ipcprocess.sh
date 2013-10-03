#!/bin/bash

#
# app
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#             Eduard Grasa <e DOT grasa AT i2cat DOT net>
#

BASE_PATH=/usr/local/rina

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BASE_PATH/lib java -jar $BASE_PATH/rinad/rina.ipcprocess.impl-1.0.0-irati-SNAPSHOT/rina.ipcprocess.impl-1.0.0-irati-SNAPSHOT.jar $1 $2 $3
