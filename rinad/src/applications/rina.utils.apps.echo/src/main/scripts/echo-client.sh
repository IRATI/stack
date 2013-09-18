#!/bin/bash

#
# app
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#             Eduard Grasa <e DOT grasa AT i2cat DOT net>
#

LP=/media/sf_irati/dist/librina/lib

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LP java -jar rina.utils.apps.echo-1.0.0-irati-SNAPSHOT.jar -role client -sdusize 30 -count 100
