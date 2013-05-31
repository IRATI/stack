#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) += rina.o

rina-y := kipcm.o efcp.o rmt.o pdufwdt.o core.o

rina-$(CONFIG_RINA_SHIM_ETH)     += shim-eth.o
rina-$(CONFIG_RINA_SHIM_TCP_UDP) += shim-tcp-udp.o
