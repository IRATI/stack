#
# Makefile for Virtual MPI (VMPI)
#
# Written by Vincenzo Maffione <v.maffione@nextworks.it>
#            Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I${src}/..
ccflags-y += -I${src}/../ipcps
ccflags-y += -Wtype-limits

EXTRA_CFLAGS := -I$(PWD)/../../include

obj-m += vmpi-provider.o

obj-$(CONFIG_VMPI_KVM_GUEST) += vmpi-kvm-guest.o

vmpi-kvm-guest-y :=						\
	vmpi-guest-impl-kvm.o vmpi-guest.o			\
	vmpi-iovec.o vmpi-stats.o

obj-$(CONFIG_VMPI_KVM_HOST) += vmpi-kvm-host.o

vmpi-kvm-host-y :=						\
	vmpi-host-impl-kvm.o vmpi-host.o			\
	vmpi-iovec.o vmpi-stats.o

ifdef NO
obj-$(CONFIG_VMPI_XEN_GUEST) += vmpi-xen-guest.o

vmpi-xen-guest-y :=						\
	vmpi-guest-impl-xen.o vmpi-guest.o			\
	vmpi-iovec.o vmpi-stats.o

obj-$(CONFIG_VMPI_XEN_HOST) += vmpi-xen-host.o

vmpi-xen-host-y :=						\
	vmpi-host-impl-xen.o  xen-mpi-back-interface.o		\
	xen-mpi-back-xenbus.o vmpi-host.o			\
	vmpi-iovec.o vmpi-stats.o
endif
