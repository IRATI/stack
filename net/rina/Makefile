#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) += rina.o

rina-y := core.o syscalls.o

rina-$(CONFIG_RINA_SYSFS) += sysfs.o

rina-y += kipcm.o efcp.o rmt.o pdufwdt.o

rina-$(CONFIG_RINA_SHIM_ETH)     += shims/shim-eth.o
rina-$(CONFIG_RINA_SHIM_TCP_UDP) += shims/shim-tcp-udp.o
