#!/bin/bash
#
# Patch the kernel in order to make it work for vWall testbed.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#
PRISTINE=/pristine

cd $PRISTINE/linux
cat drivers/net/ethernet/intel/igb/e1000_defines.h | grep ETHERNET_IEEE_VLAN_TYPE
cat drivers/net/ethernet/intel/ixgbe/ixgbe_type.h | grep IXGBE_ETHERNET_IEEE_VLAN_TYPE
cat drivers/net/ethernet/intel/e1000/e1000_hw.h | grep ETHERNET_IEEE_VLAN_TYPE
cat include/uapi/linux/if_ether.h | grep 8021Q
