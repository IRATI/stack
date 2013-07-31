#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o core.o personality.o	\
	syscalls.o netlink.o netlink-utils.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina_personality_default.o

rina_personality_default-y :=			\
	ipcp-factories.o ipcp.o kipcm.o		\
	du.o sdu.o dtcp.o dtp.o efcp.o rmt.o	\
	default.o shim-utils.o 

ifeq  ($(CONFIG_RINA_PERSONALITY_DEFAULT),m)
rina_personality_default-y +=			\
	 utils-workarounds.o
endif

obj-$(CONFIG_RINA_SHIM_EMPTY)    += shim-empty.o
obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
