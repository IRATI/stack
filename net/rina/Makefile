#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) += core.o personality.o syscalls.o netlink.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) += rina_personality_default.o

rina_personality_default-y :=				\
	shim.o kipcm.o efcp.o rmt.o default.o

obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
