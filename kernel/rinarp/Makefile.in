#
# Makefile for the RINA ARP (RINARP) layer
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I${src}/..
ccflags-y += -Wtype-limits -DCONFIG_RINARP=y
ifeq ($(REGRESSION_TESTS),y)
ccflags-y += -DCONFIG_ARP826_REGRESSION_TESTS
endif

EXTRA_CFLAGS := -I$(PWD)/../include

obj-m += arp826.o
arp826-y :=				\
	arp826-tables.o				\
	arp826-rxtx.o				\
	arp826-arm.o				\
	arp826-maps.o				\
	arp826-utils.o				\
	arp826-core.o

obj-m += rinarp.o
