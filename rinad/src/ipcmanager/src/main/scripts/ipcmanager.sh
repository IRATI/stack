#!/bin/bash

#
# app
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#             Eduard Grasa <e DOT grasa AT i2cat DOT net>
#

LP=/usr/local/rina/lib

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LP java -jar rina.ipcmanager.impl-1.0.0-irati-SNAPSHOT.jar
