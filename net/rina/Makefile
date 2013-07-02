#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) +=							\
	debug.o core.o personality.o syscalls.o netlink.o

# FIXME: utils.o should be linked to the core instead of to the personality
obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	utils.o rina_personality_default.o

rina_personality_default-y :=				\
	shim.o kipcm.o efcp.o rmt.o default.o

obj-$(CONFIG_RINA_SHIM_EMPTY)    += shim-empty.o
obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
