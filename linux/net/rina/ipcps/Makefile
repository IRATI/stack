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
obj-$(CONFIG_RINA_SHIM_HV_VIRTIO) += shim-hv-virtio.o
shim-hv-virtio-y                  := vmpi-ll-virtio.o vmpi-hl.o shim-hv.o
obj-$(CONFIG_RINA_SHIM_HV_XEN)    += shim-hv-xen.o
shim-hv-xen-y                     := vmpi-ll-virtio.o vmpi-hl.o shim-hv.o
