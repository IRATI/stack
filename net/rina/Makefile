#
# Makefile for the Linux RINA layer.
#

# built-ins
obj-y := core.o syscalls.o netlink.o
obj-$(CONFIG_RINA_SYSFS) += sysfs.o
obj-m :=

# modules

rina-$(CONFIG_RINA)              += kipcm.o efcp.o rmt.o pdufwdt.o
rina-$(CONFIG_RINA_SHIM_ETH)     += shim-eth.o
rina-$(CONFIG_RINA_SHIM_TCP_UDP) += shim-tcp-udp.o
