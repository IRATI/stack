#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o core.o personality.o	\
	syscalls.o netlink.o netlink-utils.o	\
        du.o ipcp-utils.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina_personality_default.o

rina_personality_default-y :=			\
	ipcp-factories.o ipcp.o			\
	dtcp.o dtp.o efcp.o rmt.o		\
	flow-manager.o				\
	kipcm-utils.o kipcm.o			\
	default.o

ifeq  ($(CONFIG_RINA_PERSONALITY_DEFAULT),m)
rina_personality_default-y +=			\
	 utils-workarounds.o
endif

obj-$(CONFIG_RINA_ARP_OLD)       += rina-arp-old.o
obj-$(CONFIG_RINA_ARP_NEW)       += rina-arp-new.o

obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
