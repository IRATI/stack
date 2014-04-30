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

obj-$(CONFIG_RINA_SHIM_HV_VIRTIO) += vmpi-guest-virtio.o
obj-$(CONFIG_RINA_SHIM_HV_VIRTIO) += vmpi-host-virtio.o

vmpi-guest-virtio-y		  := vmpi-guest-impl-virtio.o vmpi-guest-test.o vmpi-guest.o vmpi-structs.o vmpi-iovec.o
vmpi-host-virtio-y		  := vmpi-host-impl-virtio.o  vmpi-host-test.o  vmpi-host.o  vmpi-structs.o vmpi-iovec.o

#obj-$(CONFIG_RINA_SHIM_HV_XEN)    += shim-hv-xen.o
#shim-hv-xen-y                     := shim-hv.o
