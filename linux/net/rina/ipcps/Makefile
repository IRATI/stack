#
# Makefile for RINA IPC Processes (IPCPS)
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -I$(srctree)/net/rina/vmpi
ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA_NORMAL_IPCP)    += normal-ipcp.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN)  += shim-eth-vlan.o
shim-eth-vlan-y :=				\
	shim-eth-vlan-core.o shim-eth-utils.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)   += shim-tcp-udp.o
obj-$(CONFIG_RINA_SHIM_HV)        += shim-hv.o
