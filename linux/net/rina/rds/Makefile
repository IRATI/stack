#
# Makefile for the RINA Data Structures (RDS) layer
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA) +=				\
	 rstr.o					\
	 rmem.o					\
	 rmap.o					\
	 rwq.o					\
	 rbmp.o					\
	 rqueue.o				\
	 rfifo.o				\
	 rref.o					\
	 rtimer.o				\
	 robjects.o				\
	 rds.o
