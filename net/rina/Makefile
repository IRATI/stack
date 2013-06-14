#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA)              += core.o personality.o syscalls.o netlink.o

obj-$(CONFIG_RINA_PERSONALITY)  += rina_personality.o

rina_personality-y :=			\
	shim.o kipcm.o efcp.o rmt.o pdufwdt.o rina.o

obj-$(CONFIG_RINA_SHIM_ETH)     += shim-eth.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP) += shim-tcp-udp.o
