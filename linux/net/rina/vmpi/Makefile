#
# Makefile for Virtual MPI (VMPI)
#
# Written by Vincenzo Maffione <v.maffione@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -I$(srctree)/net/rina/ipcps
ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA_SHIM_HV_VIRTIO_GUEST) += vmpi-virtio-guest.o
obj-$(CONFIG_RINA_SHIM_HV_VIRTIO_HOST)  += vmpi-virtio-host.o

#ifeq ($(CONFIG_RINA_SHIM_HV_VIRTIO_GUEST),m)
vmpi-virtio-guest-y := \
	vmpi-guest-impl-virtio.o vmpi-guest-test.o vmpi-guest.o \
	vmpi-structs.o vmpi-iovec.o
#endif

#ifeq ($(CONFIG_RINA_SHIM_HV_VIRTIO_HOST),m)
vmpi-virtio-host-y  := \
	vmpi-host-impl-virtio.o  vmpi-host-test.o  vmpi-host.o \
	vmpi-structs.o vmpi-iovec.o
#endif
