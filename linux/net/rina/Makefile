#
# Makefile for RINA
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o				\
	rds/					\
	core.o personality.o syscalls.o		\
	rnl-utils.o rnl-workarounds.o rnl.o 	\
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

obj-$(CONFIG_RINA) += rinarp/
obj-$(CONFIG_RINA) += ipcps/
obj-$(CONFIG_RINA) += ipcps/
obj-$(CONFIG_RINA) += ipcps/
obj-$(CONFIG_RINA) += ipcps/
