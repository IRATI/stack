#!/bin/bash
#
# Prepare to VLAN testing in a generic case.
# This script need 3 arguments, which are:
# 1 - the IPC Manager configuration file.
# 2 - the eth interface to use.
# 3 - the vlan id to use.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#
VLAN=$1
ETH=$2
SANDBOX=/pristine/userspace
CONF=$3

# Check if the arguments are enough.
#
if [ $n -ne 3 ]; then
	echo "Error: I need 3 arguments, ipcm config path, interface name and vlan id."
fi

# Configure and wake up VLAN over the desired eth interface.
#
ip link add link $ETH name $ETH.$VLAN type vlan id $VLAN
ip link set dev $ETH up
ip link set dev $ETH.$VLAN up

# Load RINA modules.
#
modprobe shim-eth-vlan
modprobe rina-default-plugin
modprobe normal-ipcp

# Run the IPC Manager if the right configuration file exists.
#
if [ -f $CONF ]; then
	$SANDBOX/bin/ipcm -c $CONF
else
	echo "Error: no IPC Manager configuration file found!"
fi
