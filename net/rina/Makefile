#
# Makefile for the Linux RINA layer.
#

# built-ins
obj-$(CONFIG_RINA)       += core.o personality.o syscalls.o netlink.o
obj-$(CONFIG_RINA_SYSFS) += sysfs.o
obj-$(CONFIG_RINA_PERSONALITY) += rina_personality.o

# modules
rina_personality-$(RINA_PERSONALITY) += kipcm.o efcp.o rmt.o pdufwdt.o rina.o
rina_personality-$(CONFIG_RINA_SHIM_ETH)     += shim-eth.o
rina_personality-$(CONFIG_RINA_SHIM_TCP_UDP) += shim-tcp-udp.o
