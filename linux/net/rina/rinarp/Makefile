#
# Makefile for the RINA ARP (RINARP) layer
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -Wtype-limits

obj-$(CONFIG_ARP826) += arp826.o
arp826-y             :=				\
	arp826-tables.o				\
	arp826-rxtx.o				\
	arp826-arm.o				\
	arp826-maps.o				\
	arp826-utils.o				\
	arp826-core.o

obj-$(CONFIG_RINARP) += 			\
	rinarp.o
