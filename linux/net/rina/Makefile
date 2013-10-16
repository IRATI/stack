#
# Makefile for the Linux RINA layer.
#

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o core.o personality.o	\
	syscalls.o				\
	rnl.o rnl-utils.o rnl-workarounds.o	\
	du.o ipcp-utils.o common.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina_personality_default.o

rina_personality_default-y :=			\
	ipcp-factories.o ipcp.o			\
	qos.o					\
	dtcp.o dtp.o efcp-utils.o cidm.o efcp.o	\
	pft.o rmt.o				\
	fidm.o kfa-utils.o kfa.o		\
	kipcm-utils.o kipcm.o			\
	default.o

ifeq  ($(CONFIG_RINA_PERSONALITY_DEFAULT),m)
rina_personality_default-y +=			\
	 utils-workarounds.o
endif

obj-$(CONFIG_RINARP) += rinarp.o

obj-$(CONFIG_ARP826) += arp826.o
arp826-y             :=				\
	arp826-tables.o				\
	arp826-arm.o				\
	arp826-utils.o				\
	arp826-core.o

obj-$(CONFIG_RINA_NORMAL_IPCP)   += normal-ipcp.o
obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
