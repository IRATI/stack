#
# Makefile for RINA
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA) +=						\
	utils.o debug.o						\
	rds/							\
	core.o							\
	rnl-utils.o rnl-workarounds.o rnl.o			\
	buffer.o pci.o pdu.o sdu.o du.o	        		\
	serdes.o pdu-ser.o					\
	ipcp-utils.o						\
	connection.o common.o policies.o			\
	dtp-conf-utils.o dtcp-conf-utils.o      		\
	ipcp-factories.o ipcp-instances.o			\
	cidm.o dt.o dt-utils.o dtcp.o dtp.o efcp-utils.o efcp.o	\
	pff.o rmt.o						\
	pim.o pidm.o kfa-utils.o kfa.o				\
	kipcm-utils.o kipcm.o					\
	sdup.o							\
	ps-factory.o						\
	rmt-ps-default.o					\
	dtp-ps-default.o					\
	dtcp-ps-default.o                       		\
	pff-ps-default.o					\
	sdup-crypto-ps-default.o				\
	sdup-errc-ps-default.o					\
	sdup-ttl-ps-default.o				

obj-y +=							\
	syscalls.o

obj-m +=							\
	rina-default-plugin.o
	
rina-default-plugin-y :=					\
	default-plugin.o

obj-$(CONFIG_VMPI) += vmpi/
obj-$(CONFIG_RINA) += ipcps/
obj-$(CONFIG_RINA) += rinarp/
