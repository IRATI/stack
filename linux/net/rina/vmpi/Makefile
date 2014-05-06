#
# Makefile for Virtual MPI (VMPI)
#
# Written by Vincenzo Maffione <v.maffione@nextworks.it>
#            Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -I$(srctree)/net/rina/ipcps
ccflags-y += -Wtype-limits

obj-$(CONFIG_VMPI_VIRTIO_GUEST) += vmpi-virtio-guest.o

vmpi-virtio-guest-y :=						\
	vmpi-guest-impl-virtio.o vmpi-guest-test.o vmpi-guest.o	\
	vmpi-structs.o vmpi-iovec.o

obj-$(CONFIG_VMPI_VIRTIO_HOST) += vmpi-virtio-host.o

vmpi-virtio-host-y :=						\
	vmpi-host-impl-virtio.o  vmpi-host-test.o  vmpi-host.o	\
	vmpi-structs.o vmpi-iovec.o
