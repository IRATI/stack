#
# Makefile for RINA IPC Processes (IPCPS)
#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ccflags-y += -I$(srctree)/net/rina
ccflags-y += -Wtype-limits

obj-$(CONFIG_RINA_NORMAL_IPCP)    += normal-ipcp.o
obj-$(CONFIG_RINA_SHIM_DUMMY)     += shim-dummy.o
obj-$(CONFIG_RINA_SHIM_ETH_VLAN)  += shim-eth-vlan.o
obj-$(CONFIG_RINA_SHIM_TCP_UDP)   += shim-tcp-udp.o

obj-m				  += shim-hv.o
shim-hv-y			  := shim-hv-main.o

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
