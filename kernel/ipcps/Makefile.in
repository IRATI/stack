#
# Makefile for RINA IPC Processes (IPCPS)
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(src)/..
ccflags-y += -I$(src)../vmpi
ccflags-y += -Wtype-limits
ccflags-y += -DCONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE=$(TCP_UDP_BUFFER_SIZE)
ifeq ($(REGRESSION_TESTS),y)
ccflags-y += -DCONFIG_RINA_SHIM_ETH_VLAN_REGRESSION_TESTS
endif

EXTRA_CFLAGS := -I$(PWD)/../include

obj-m  += normal-ipcp.o
obj-m  += shim-eth-vlan.o
shim-eth-vlan-y :=				\
	shim-eth-vlan-core.o
obj-m   += shim-tcp-udp.o

ifeq ($(HAVE_VMPI),y)
obj-m   += shim-hv.o
shim-hv-y := shim-hv-core.o vmpi-bufs.o
endif
