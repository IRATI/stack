#
# Makefile for RINA
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA) +=				\
	utils.o debug.o				\
	rds/					\
	core.o personality.o			\
	rnl-utils.o rnl-workarounds.o rnl.o	\
	buffer.o qos.o pci.o pdu.o sdu.o du.o	\
	serdes.o pdu-ser.o du-protection.o	\
	ipcp-utils.o				\
	connection.o common.o policies.o	\
	dtp-conf-utils.o dtcp-conf-utils.o      \

obj-y +=					\
	syscalls.o

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina-personality-default.o

rina-personality-default-y :=					\
	ipcp-factories.o ipcp-instances.o			\
	cidm.o dt.o dt-utils.o dtcp.o dtp.o efcp-utils.o efcp.o	\
	pff.o rmt.o						\
	pim.o pidm.o kfa-utils.o kfa.o				\
	kipcm-utils.o kipcm.o					\
	default.o ps-factory.o

ifeq  ($(CONFIG_RINA_PERSONALITY_DEFAULT),m)
rina-personality-default-y +=			\
	 utils-workarounds.o
endif

obj-$(CONFIG_RINA_PERSONALITY_DEFAULT) +=	\
	rina-default-plugin.o

rina-default-plugin-y :=			\
	default-plugin.o			\
	rmt-ps-common.o				\
	rmt-ps-default.o			\
	dtp-ps-common.o				\
	dtp-ps-default.o			\
	dtcp-ps-common.o 		        \
	dtcp-ps-default.o                       \
	pff-ps-default.o

obj-$(CONFIG_VMPI) += vmpi/
obj-$(CONFIG_RINA) += ipcps/
obj-$(CONFIG_RINA) += rinarp/
