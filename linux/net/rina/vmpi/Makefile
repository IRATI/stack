#
# Makefile for Virtual MPI (VMPI)
#
# Written by Vincenzo Maffione <v.maffione@nextworks.it>
#            Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -I$(srctree)/net/rina/ipcps
ccflags-y += -Wtype-limits

obj-$(CONFIG_VMPI_KVM_GUEST) += vmpi-kvm-guest.o

vmpi-kvm-guest-y :=						\
	vmpi-guest-impl-kvm.o vmpi-test.o vmpi-guest.o	\
	vmpi-structs.o vmpi-iovec.o

obj-$(CONFIG_VMPI_KVM_HOST) += vmpi-kvm-host.o

vmpi-kvm-host-y :=						\
	vmpi-host-impl-kvm.o  vmpi-test.o  vmpi-host.o	\
	vmpi-structs.o vmpi-iovec.o

obj-$(CONFIG_VMPI_XEN_GUEST) += vmpi-xen-guest.o

vmpi-xen-guest-y :=						\
	xen-mpi-front.o vmpi-structs.o vmpi-guest.o		\
	vmpi-iovec.o vmpi-test.o

obj-$(CONFIG_VMPI_XEN_HOST) += vmpi-xen-host.o

vmpi-xen-host-y :=						\
	xen-mpi-back.o  xen-mpi-back-interface.o		\
	xen-mpi-back-xenbus.o vmpi-structs.o vmpi-host.o	\
	vmpi-iovec.o vmpi-test.o
