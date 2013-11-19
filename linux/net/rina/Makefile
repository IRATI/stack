#
# Makefile for the Linux RINA layer
#

ccflags-y := -I./

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o				\
	rds/rwq.o rds/rmem.o rds/rmap.o		\
	core.o personality.o syscalls.o		\
	rnl.o rnl-utils.o rnl-workarounds.o	\
	du.o ipcp-utils.o common.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina_personality_default.o

rina_personality_default-y :=			\
	ipcp-factories.o ipcp.o			\
	qos.o					\
	cidm.o dtcp.o dtp.o efcp-utils.o efcp.o	\
	pft.o rmt.o				\
	pidm.o kfa-utils.o kfa.o		\
	kipcm-utils.o kipcm.o			\
	default.o

ifeq  ($(CONFIG_RINA_PERSONALITY_DEFAULT),m)
rina_personality_default-y +=			\
	 utils-workarounds.o
endif

obj-$(CONFIG_RINARP)             += rinarp/

obj-$(CONFIG_RINA_NORMAL_IPCP)   += normal-ipcp.o
obj-$(CONFIG_RINA_SHIM_DUMMY)    += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN) += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)  += shim-tcp-udp.o
